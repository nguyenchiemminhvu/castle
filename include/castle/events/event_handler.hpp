#ifndef CASTLE_EVENTS_EVENT_HANDLER_HPP
#define CASTLE_EVENTS_EVENT_HANDLER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/heap.hpp"
#include "castle/container/ring_buffer.hpp"
#include "castle/chrono/chrono.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/tuple.hpp"
#include "castle/callbacks/inplace_function.hpp"

#if CASTLE_USING_PTHREAD

#include <errno.h>
#include <pthread.h>

namespace castle
{
namespace events
{

// ============================================================================
// event_handler
// ----------------------------------------------------------------------------
// Fixed-capacity asynchronous event handler for POSIX pthread targets.
//
// Every accepted event consumes exactly one event slot until it is executed.
// Delayed and repeated events use the same bounded storage. A repeated event is
// rescheduled in-place instead of expanding into N independent queue entries.
//
// The handler owns one dedicated pthread. Callbacks are stored in
// callbacks::inplace_function, so the callable's maximum size is known at
// compile time and does not require heap allocation.
//
// Usage:
//
//   castle::events::event_handler<16> handler;
//
//   handler.post_event([] {
//       // Execute asynchronously.
//   });
//
//   handler.post_event(process_message, 42U, 128U);
//
//   handler.post_delayed_event(100, [] {
//       // Execute after approximately 100 ms.
//   });
//
//   handler.post_repeated_event(5, 250, [] {
//       // Execute 5 times, once every 250 ms.
//   });
//
// Threading contract:
//   - post_*() may be called concurrently from multiple threads.
//   - The handler destructor must not run concurrently with post_*().
//   - Callback code runs on the handler's dedicated pthread.
//   - Callback code may post additional events to this handler.
//
// Failure model:
//   - status::full means all event slots are currently occupied.
//   - status::system_call_error means a pthread primitive failed.
//   - status::invalid_argument is used for invalid repetition requests.
//   - status::not_configured is returned when the pthread could not be created.
// ============================================================================
template <
    size_type MaxEventCount,
    size_type StorageSize = castle::inplace_storage_reserved,
    size_type StorageAlignment = castle::inplace_alignment_default>
class event_handler
{
    static_assert(MaxEventCount > 0U,
                  "event_handler MaxEventCount must be greater than zero");

public:
    using status_type = castle::status;
    using task_type = castle::callbacks::inplace_function<void(), StorageSize, StorageAlignment>;

private:
    // ------------------------------------------------------------------------
    // Timestamp helpers.
    // ------------------------------------------------------------------------
    using steady_clock = castle::chrono::steady_clock;
    using time_point = typename steady_clock::time_point;
    using milliseconds = castle::chrono::milliseconds;
    using nanoseconds = castle::chrono::nanoseconds;

    struct event_node
    {
        size_type slot = 0U;
        time_point due;
        uint64_t sequence = 0U;
    };

    struct event_node_compare
    {
        // min-heap semantics: earlier events have higher priority. Sequence
        // preserves FIFO order for events scheduled for the same timestamp.
        bool operator()(CASTLE_CONST event_node& lhs,
                        CASTLE_CONST event_node& rhs) CASTLE_CONST CASTLE_NOEXCEPT
        {
            CASTLE_CONST auto lhs_ticks = lhs.due.time_since_epoch().count();
            CASTLE_CONST auto rhs_ticks = rhs.due.time_since_epoch().count();

            if (lhs_ticks != rhs_ticks)
            {
                return lhs_ticks < rhs_ticks;
            }
            return lhs.sequence < rhs.sequence;
        }
    };

    struct event_slot
    {
        task_type task{};
        time_point due;
        uint64_t interval_ms = 0U;
        size_type repeat_remaining = 0U;
        bool repeating = false;
        bool occupied = false;
    };

    using scheduled_queue_type =
        castle::container::min_heap<event_node, MaxEventCount, event_node_compare>;
    using free_queue_type = castle::container::ring_buffer<size_type, MaxEventCount>;

    template <typename Callable, typename... Args>
    class bound_task
    {
    public:
        using callable_type = castle::decay_t<Callable>;
        using args_tuple_type = castle::tuple<castle::decay_t<Args>...>;

        bound_task(Callable&& callable, Args&&... args)
            : callable_(CASTLE_FORWARD<Callable>(callable))
            , args_(CASTLE_FORWARD<Args>(args)...)
        {
        }

        void operator()()
        {
            invoke(castle::sequence::index_sequence_for<Args...>{});
        }

    private:
        template <size_type... Indices>
        void invoke(castle::sequence::index_sequence<Indices...>)
        {
            callable_(castle::get<Indices>(args_)...);
        }

        callable_type callable_;
        args_tuple_type args_;
    };

    template <typename T, typename... Args>
    class member_bound_task
    {
    public:
        using member_type = void (T::*)(Args...);
        using args_tuple_type = castle::tuple<castle::decay_t<Args>...>;

        member_bound_task(T* object, member_type member, Args&&... args)
            : object_(object)
            , member_(member)
            , args_(CASTLE_FORWARD<Args>(args)...)
        {
        }

        void operator()()
        {
            invoke(castle::sequence::index_sequence_for<Args...>{});
        }

    private:
        template <size_type... Indices>
        void invoke(castle::sequence::index_sequence<Indices...>)
        {
            (object_->*member_)(castle::get<Indices>(args_)...);
        }

        T* object_;
        member_type member_;
        args_tuple_type args_;
    };

public:
    // ------------------------------------------------------------------------
    // Construction / destruction.
    // ------------------------------------------------------------------------
    event_handler() CASTLE_NOEXCEPT
        : thread_()
        , sequence_counter_(0U)
        , running_(false)
        , initialized_(false)
        , thread_created_(false)
        , mutex_initialized_(false)
        , condition_initialized_(false)
    {
        initialize_free_slots();

        if (!initialize_synchronization())
        {
            return;
        }

        // Publish the running state before pthread_create(). POSIX permits the
        // new thread to begin execution immediately, so setting this flag only
        // after pthread_create() would introduce a startup race.
        running_ = true;

        CASTLE_CONST int create_result =
            pthread_create(&thread_, nullptr, &event_handler::thread_entry, this);

        if (create_result != 0)
        {
            running_ = false;
            destroy_synchronization();
            return;
        }

        initialized_ = true;
        thread_created_ = true;
    }

    event_handler(CASTLE_CONST event_handler&) CASTLE_DELETE;
    event_handler& operator=(CASTLE_CONST event_handler&) CASTLE_DELETE;

    event_handler(event_handler&&) CASTLE_DELETE;
    event_handler& operator=(event_handler&&) CASTLE_DELETE;

    ~event_handler()
    {
        static_cast<void>(shutdown());
    }

    // ------------------------------------------------------------------------
    // Capacity and state queries.
    // ------------------------------------------------------------------------
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return MaxEventCount;
    }

    static CASTLE_CONSTEXPR size_type callback_storage_size() CASTLE_NOEXCEPT
    {
        return StorageSize;
    }

    static CASTLE_CONSTEXPR size_type callback_storage_alignment() CASTLE_NOEXCEPT
    {
        return StorageAlignment;
    }

    bool initialized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return initialized_;
    }

    bool running() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!mutex_initialized_ || pthread_mutex_lock(&mutex_) != 0)
        {
            return false;
        }

        CASTLE_CONST bool result = running_;
        pthread_mutex_unlock(&mutex_);
        return result;
    }

    size_type pending() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!mutex_initialized_ || pthread_mutex_lock(&mutex_) != 0)
        {
            return 0U;
        }

        CASTLE_CONST size_type result = scheduled_queue_.size();
        pthread_mutex_unlock(&mutex_);
        return result;
    }

    size_type available() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!mutex_initialized_ || pthread_mutex_lock(&mutex_) != 0)
        {
            return 0U;
        }

        CASTLE_CONST size_type result = free_queue_.size();
        pthread_mutex_unlock(&mutex_);
        return result;
    }

    // ------------------------------------------------------------------------
    // Immediate event.
    // ------------------------------------------------------------------------
    // Post a callable with no arguments for immediate asynchronous execution.
    template <typename Callable>
    status_type post_event(Callable&& callable) CASTLE_NOEXCEPT
    {
        return post_bound(
            task_type(CASTLE_FORWARD<Callable>(callable)),
            0U,
            false,
            0U
        );
    }

    // Post a callable with arguments for immediate asynchronous execution.
    template <typename Callable, typename... Args>
    status_type post_event(Callable&& callable, Args&&... args) CASTLE_NOEXCEPT
    {
        using bound_type = bound_task<Callable, Args...>;

        return post_bound(
            task_type(bound_type(
                CASTLE_FORWARD<Callable>(callable),
                CASTLE_FORWARD<Args>(args)...)),
            0U,
            false,
            0U
        );
    }

    // ------------------------------------------------------------------------
    // Delayed event.
    // ------------------------------------------------------------------------
    // Schedule one asynchronous execution after delay_ms milliseconds.
    template <typename Callable>
    status_type post_delayed_event(
        uint64_t delay_ms,
        Callable&& callable) CASTLE_NOEXCEPT
    {
        return post_bound(
            task_type(CASTLE_FORWARD<Callable>(callable)),
            delay_ms,
            false,
            0U
        );
    }

    // Schedule one callable with arguments after delay_ms milliseconds.
    template <typename Callable, typename... Args>
    status_type post_delayed_event(
        uint64_t delay_ms,
        Callable&& callable,
        Args&&... args) CASTLE_NOEXCEPT
    {
        using bound_type = bound_task<Callable, Args...>;

        return post_bound(
            task_type(bound_type(
                CASTLE_FORWARD<Callable>(callable),
                CASTLE_FORWARD<Args>(args)...)),
            delay_ms,
            false,
            0U
        );
    }

    // ------------------------------------------------------------------------
    // Repeated event.
    // ------------------------------------------------------------------------
    // Schedule one callable a fixed number of times. The first invocation is
    // scheduled immediately, matching libeventcpp::post_repeated_event().
    // The event slot is reused between invocations, so this consumes exactly
    // one slot regardless of the repetition count.
    template <typename Callable>
    status_type post_repeated_event(
        size_type times,
        uint64_t duration_ms,
        Callable&& callable) CASTLE_NOEXCEPT
    {
        if (times == 0U)
        {
            return status_type::ok;
        }

        return post_bound(
            task_type(CASTLE_FORWARD<Callable>(callable)),
            0U,
            true,
            duration_ms,
            times
        );
    }

    // Schedule a callable with arguments a fixed number of times.
    template <typename Callable, typename... Args>
    status_type post_repeated_event(
        size_type times,
        uint64_t duration_ms,
        Callable&& callable,
        Args&&... args) CASTLE_NOEXCEPT
    {
        if (times == 0U)
        {
            return status_type::ok;
        }

        using bound_type = bound_task<Callable, Args...>;

        return post_bound(
            task_type(bound_type(
                CASTLE_FORWARD<Callable>(callable),
                CASTLE_FORWARD<Args>(args)...)),
            0U,
            true,
            duration_ms,
            times
        );
    }

    // ------------------------------------------------------------------------
    // Member-function convenience overloads.
    // ------------------------------------------------------------------------
    // The object is intentionally supplied explicitly. Unlike libeventcpp,
    // castle::events::event_handler does not require shared ownership or RTTI, so the
    // event handler never silently extends the lifetime of a target object.
    template <typename T, typename... Args>
    status_type post_event(
        void (T::*member)(Args...),
        T* object,
        Args&&... args) CASTLE_NOEXCEPT
    {
        return post_event(object, member, CASTLE_FORWARD<Args>(args)...);
    }

    template <typename T, typename... Args>
    status_type post_delayed_event(
        uint64_t delay_ms,
        void (T::*member)(Args...),
        T* object,
        Args&&... args) CASTLE_NOEXCEPT
    {
        return post_delayed_event(
            delay_ms, object, member, CASTLE_FORWARD<Args>(args)...
        );
    }

    template <typename T, typename... Args>
    status_type post_repeated_event(
        size_type times,
        uint64_t duration_ms,
        void (T::*member)(Args...),
        T* object,
        Args&&... args) CASTLE_NOEXCEPT
    {
        return post_repeated_event(
            times, duration_ms, object, member, CASTLE_FORWARD<Args>(args)...
        );
    }

    template <typename T, typename... Args>
    status_type post_event(
        T* object,
        void (T::*member)(Args...),
        Args&&... args) CASTLE_NOEXCEPT
    {
        if (object == nullptr || member == nullptr)
        {
            return status_type::invalid_argument;
        }

        using member_task_type = member_bound_task<T, Args...>;

        return post_bound(
            task_type(member_task_type(
                object,
                member,
                CASTLE_FORWARD<Args>(args)...)),
            0U,
            false,
            0U
        );
    }

    template <typename T, typename... Args>
    status_type post_delayed_event(
        uint64_t delay_ms,
        T* object,
        void (T::*member)(Args...),
        Args&&... args) CASTLE_NOEXCEPT
    {
        if (object == nullptr || member == nullptr)
        {
            return status_type::invalid_argument;
        }

        using member_task_type = member_bound_task<T, Args...>;

        return post_bound(
            task_type(member_task_type(
                object,
                member,
                CASTLE_FORWARD<Args>(args)...)),
            delay_ms,
            false,
            0U
        );
    }

    template <typename T, typename... Args>
    status_type post_repeated_event(
        size_type times,
        uint64_t duration_ms,
        T* object,
        void (T::*member)(Args...),
        Args&&... args) CASTLE_NOEXCEPT
    {
        if (times == 0U)
        {
            return status_type::ok;
        }

        if (object == nullptr || member == nullptr)
        {
            return status_type::invalid_argument;
        }

        using member_task_type = member_bound_task<T, Args...>;

        return post_bound(
            task_type(member_task_type(
                object,
                member,
                CASTLE_FORWARD<Args>(args)...)),
            0U,
            true,
            duration_ms,
            times
        );
    }

    // ------------------------------------------------------------------------
    // Explicit shutdown.
    // ------------------------------------------------------------------------
    // Stop the looper thread and discard all pending events. shutdown() is
    // idempotent. It must not be called by a callback executing on this same
    // handler thread because pthread_join() cannot join the calling thread.
    status_type shutdown() CASTLE_NOEXCEPT
    {
        if (!mutex_initialized_)
        {
            return status_type::ok;
        }

        if (pthread_mutex_lock(&mutex_) != 0)
        {
            return status_type::system_call_error;
        }

        if (!thread_created_)
        {
            pthread_mutex_unlock(&mutex_);
            initialized_ = false;
            destroy_synchronization();
            return status_type::ok;
        }

        running_ = false;
        scheduled_queue_.clear();

        // Do not destroy event slots yet. The worker may currently be executing
        // one of their callbacks. Joining first guarantees that callback has
        // returned before its inplace_function storage is destroyed.
        CASTLE_CONST int broadcast_result = pthread_cond_broadcast(&condition_);
        pthread_mutex_unlock(&mutex_);

        if (pthread_equal(pthread_self(), thread_) != 0)
        {
            // Self-shutdown is not supported because pthread_join() would
            // deadlock. Leave the thread to return naturally.
            return status_type::system_call_error;
        }

        CASTLE_CONST int join_result = pthread_join(thread_, nullptr);

        // The worker is no longer touching the event slots after a successful
        // join, so pending and currently executing callback storage can now be
        // reset safely.
        if (pthread_mutex_lock(&mutex_) == 0)
        {
            release_all_slots_locked();
            pthread_mutex_unlock(&mutex_);
        }

        thread_created_ = false;
        initialized_ = false;
        destroy_synchronization();

        return (broadcast_result == 0 && join_result == 0)
               ? status_type::ok
               : status_type::system_call_error;
    }

    status_type stop() CASTLE_NOEXCEPT
    {
        return shutdown();
    }

    bool is_running() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return running();
    }

private:
    // ------------------------------------------------------------------------
    // POSIX clock / timespec helpers.
    // ------------------------------------------------------------------------
    static time_point now() CASTLE_NOEXCEPT
    {
        return steady_clock::now();
    }

    static time_point add_ms(
        CASTLE_CONST time_point& timestamp,
        uint64_t value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST milliseconds delta_ms(static_cast<int64_t>(value));
        CASTLE_CONST auto delta =
            castle::chrono::duration_cast<typename time_point::duration>(delta_ms);

        time_point result(timestamp);
        result += delta;
        return result;
    }

    static bool to_timespec(
        CASTLE_CONST time_point& value,
        struct timespec& out) CASTLE_NOEXCEPT
    {
        using epoch_duration = typename time_point::duration;

        CASTLE_CONST int64_t count = value.time_since_epoch().count();
        if (count < 0)
        {
            return false;
        }

        if ((epoch_duration::period::num == castle::nano::num) &&
            (epoch_duration::period::den == castle::nano::den))
        {
            out.tv_sec = static_cast<time_t>(count / 1000000000LL);
            out.tv_nsec = static_cast<long>(count % 1000000000LL);
            return true;
        }

        // CASTLE's standard non-nanosecond periods all use num == 1.
        // Split into whole seconds before scaling the fractional remainder so
        // the conversion never multiplies a long-running absolute timestamp.
        if (epoch_duration::period::num == 1LL)
        {
            CASTLE_CONST int64_t denominator = epoch_duration::period::den;
            CASTLE_CONST int64_t whole_seconds = count / denominator;
            CASTLE_CONST int64_t remainder = count % denominator;

            if (denominator <= 0 || whole_seconds < 0 || remainder < 0)
            {
                return false;
            }

            out.tv_sec = static_cast<time_t>(whole_seconds);
            out.tv_nsec = static_cast<long>((remainder * 1000000000LL) / denominator);
            return true;
        }

        return false;
    }

    // ------------------------------------------------------------------------
    // Synchronization initialization.
    // ------------------------------------------------------------------------
    bool initialize_synchronization() CASTLE_NOEXCEPT
    {
        if (pthread_mutex_init(&mutex_, nullptr) != 0)
        {
            return false;
        }
        mutex_initialized_ = true;

        pthread_condattr_t attributes;
        if (pthread_condattr_init(&attributes) != 0)
        {
            pthread_mutex_destroy(&mutex_);
            mutex_initialized_ = false;
            return false;
        }

        CASTLE_CONST int clock_result =
            pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC);

        if (clock_result != 0 || pthread_cond_init(&condition_, &attributes) != 0)
        {
            pthread_condattr_destroy(&attributes);
            pthread_mutex_destroy(&mutex_);
            mutex_initialized_ = false;
            return false;
        }

        pthread_condattr_destroy(&attributes);
        condition_initialized_ = true;
        return true;
    }

    void destroy_synchronization() CASTLE_NOEXCEPT
    {
        if (condition_initialized_)
        {
            pthread_cond_destroy(&condition_);
            condition_initialized_ = false;
        }

        if (mutex_initialized_)
        {
            pthread_mutex_destroy(&mutex_);
            mutex_initialized_ = false;
        }
    }

    void initialize_free_slots() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxEventCount; ++i)
        {
            if (!free_queue_.push(i))
            {
                CASTLE_ASSERT_FAIL(
                    CASTLE_ERROR_GENERIC("event_handler failed to initialize free event queue"));
                return;
            }
        }
    }

    void release_all_slots_locked() CASTLE_NOEXCEPT
    {
        free_queue_.clear();

        for (size_type i = 0U; i < MaxEventCount; ++i)
        {
            slots_[i].task = task_type();
            slots_[i].due = time_point();
            slots_[i].interval_ms = 0U;
            slots_[i].repeat_remaining = 0U;
            slots_[i].repeating = false;
            slots_[i].occupied = false;
            free_queue_.push(i);
        }
    }

    // ------------------------------------------------------------------------
    // Event insertion.
    // ------------------------------------------------------------------------
    status_type post_bound(
        task_type&& task,
        uint64_t delay_ms,
        bool repeating,
        uint64_t interval_ms,
        size_type repeat_count = 0U) CASTLE_NOEXCEPT
    {
        if (!task)
        {
            return status_type::invalid_callback;
        }

        if (!mutex_initialized_)
        {
            return status_type::not_configured;
        }

        if (pthread_mutex_lock(&mutex_) != 0)
        {
            return status_type::system_call_error;
        }

        if (!running_ || free_queue_.empty() || scheduled_queue_.full())
        {
            pthread_mutex_unlock(&mutex_);
            return status_type::full;
        }

        size_type slot = 0U;
        if (!free_queue_.pop(slot))
        {
            pthread_mutex_unlock(&mutex_);
            return status_type::system_call_error;
        }

        CASTLE_CONST time_point due = add_ms(now(), delay_ms);

        event_slot& target = slots_[slot];
        target.task = CASTLE_MOVE(task);
        target.due = due;
        target.interval_ms = interval_ms;
        target.repeat_remaining = repeat_count;
        target.repeating = repeating;
        target.occupied = true;

        event_node node;
        node.slot = slot;
        node.due = due;
        node.sequence = sequence_counter_++;

        CASTLE_CONST status_type queue_status = scheduled_queue_.push(node);
        if (queue_status != status_type::ok)
        {
            target.task = task_type();
            target.due = time_point();
            target.interval_ms = 0U;
            target.repeat_remaining = 0U;
            target.repeating = false;
            target.occupied = false;
            free_queue_.push(slot);
            pthread_mutex_unlock(&mutex_);
            return queue_status;
        }

        CASTLE_CONST int signal_result = pthread_cond_signal(&condition_);
        pthread_mutex_unlock(&mutex_);

        return signal_result == 0
               ? status_type::ok
               : status_type::system_call_error;
    }

    // ------------------------------------------------------------------------
    // Event loop.
    // ------------------------------------------------------------------------
    static void* thread_entry(void* arg) CASTLE_NOEXCEPT
    {
        event_handler* handler = static_cast<event_handler*>(arg);
        handler->run_loop();
        return nullptr;
    }

    void run_loop() CASTLE_NOEXCEPT
    {
        while (true)
        {
            if (pthread_mutex_lock(&mutex_) != 0)
            {
                return;
            }

            while (running_ && scheduled_queue_.empty())
            {
                if (pthread_cond_wait(&condition_, &mutex_) != 0)
                {
                    running_ = false;
                    pthread_mutex_unlock(&mutex_);
                    return;
                }
            }

            if (!running_)
            {
                pthread_mutex_unlock(&mutex_);
                return;
            }

            event_node top = scheduled_queue_.top();
            CASTLE_CONST time_point now_value = now();

            if (top.due.time_since_epoch().count() >
                now_value.time_since_epoch().count())
            {
                struct timespec timeout;
                if (!to_timespec(top.due, timeout))
                {
                    running_ = false;
                    pthread_mutex_unlock(&mutex_);
                    return;
                }
                CASTLE_CONST int wait_result =
                    pthread_cond_timedwait(&condition_, &mutex_, &timeout);

                if (wait_result != 0 && wait_result != ETIMEDOUT)
                {
                    running_ = false;
                    pthread_mutex_unlock(&mutex_);
                    return;
                }

                pthread_mutex_unlock(&mutex_);

                // A signal means the queue may have changed. Re-evaluate from
                // the top of the loop. ETIMEDOUT means the same thing except
                // that the existing top event is now ready.
                continue;
            }

            scheduled_queue_.pop();
            size_type slot = top.slot;
            event_slot& current = slots_[slot];

            // The slot remains occupied while the callback executes. This
            // prevents another producer from reclaiming the callable storage.
            task_type* task = &current.task;
            pthread_mutex_unlock(&mutex_);

            (*task)(); // Execute task outside of the lock to avoid deadlocks.

            if (pthread_mutex_lock(&mutex_) != 0)
            {
                return;
            }

            if (!current.occupied)
            {
                // Defensive guard for future cancellation support.
                pthread_mutex_unlock(&mutex_);
                continue;
            }

            if (current.repeating && current.repeat_remaining > 1U)
            {
                --current.repeat_remaining;
                current.due = add_ms(current.due, current.interval_ms);

                event_node next;
                next.slot = slot;
                next.due = current.due;
                next.sequence = sequence_counter_++;

                if (scheduled_queue_.push(next) == status_type::ok)
                {
                    pthread_cond_signal(&condition_);
                    pthread_mutex_unlock(&mutex_);
                    continue;
                }

                // Queue insertion cannot normally fail because the popped
                // event created one free queue position. Treat an invariant
                // violation as a dropped event rather than corrupting state.
            }

            current.task = task_type();
            current.due = time_point();
            current.interval_ms = 0U;
            current.repeat_remaining = 0U;
            current.repeating = false;
            current.occupied = false;
            free_queue_.push(slot);

            pthread_mutex_unlock(&mutex_);
        }
    }

private:
    pthread_t thread_;

    event_slot slots_[MaxEventCount];
    free_queue_type free_queue_;
    scheduled_queue_type scheduled_queue_;

    CASTLE_MUTABLE pthread_mutex_t mutex_;
    pthread_cond_t condition_;

    uint64_t sequence_counter_;
    bool running_;
    bool initialized_;
    bool thread_created_;
    bool mutex_initialized_;
    bool condition_initialized_;
};

} // namespace events
} // namespace castle

#else

#error "castle::events::event_handler requires CASTLE_USING_PTHREAD."

#endif // CASTLE_USING_PTHREAD
#endif // CASTLE_EVENTS_EVENT_HANDLER_HPP
