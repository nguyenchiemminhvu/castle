// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file event_handler.hpp
 * @brief Fixed-capacity asynchronous event scheduler backed by one POSIX worker thread.
 * @details Use this component when embedded or systems code needs deterministic,
 * bounded background execution of immediate, delayed, or repeated callbacks
 * without Castle-managed heap allocation. Every accepted event occupies one of
 * `MaxEventCount` event slots until it completes. Posting is internally
 * synchronized and may be done from multiple producer threads, while callbacks
 * execute serially on the handler's dedicated pthread. Repeated events reuse
 * their original slot instead of expanding into multiple queued entries.
 *
 * This header is available only when `CASTLE_USING_PTHREAD` is enabled.
 * Destruction and `shutdown()` must not race with posting, and `shutdown()`
 * must not be called from the handler's own worker thread.
 *
 * @code
 * #include "castle/events/event_handler.hpp"
 *
 * struct worker
 * {
 *     void step(uint32_t value) { total += value; }
 *     uint32_t total = 0U;
 * };
 *
 * castle::events::event_handler<8U> handler;
 * worker w;
 *
 * handler.post_event(&worker::step, &w, 1U);
 * handler.post_delayed_event(10U, &worker::step, &w, 2U);
 * handler.post_repeated_event(3U, 25U, &worker::step, &w, 4U);
 * handler.shutdown();
 * @endcode
 */
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
#include "castle/callbacks/function.hpp"

#if CASTLE_USING_PTHREAD

#include <errno.h>
#include <pthread.h>

namespace castle
{
namespace events
{

/**
 * @brief Fixed-capacity asynchronous event handler that owns one worker pthread.
 * @tparam MaxEventCount Maximum number of events that may be pending or executing.
 * @tparam StorageSize Inline storage size reserved for each bound callback.
 * @tparam StorageAlignment Alignment of the inline callback storage.
 * @note Immediate, delayed, and repeated events all consume from the same fixed slot pool.
 * @warning This class requires `CASTLE_USING_PTHREAD` and uses POSIX synchronization primitives.
 */
template <
    size_type MaxEventCount,
    size_type StorageSize = castle::inplace_storage_reserved,
    size_type StorageAlignment = castle::inplace_alignment_default>
class event_handler
{
    static_assert(MaxEventCount > 0U,
                  "event_handler MaxEventCount must be greater than zero");

public:
    /**
     * @brief Status type returned by posting and shutdown operations.
     */
    using status_type = castle::status;

    /**
     * @brief Stored task wrapper used internally for bound `void()` callbacks.
     */
    using task_type = castle::callbacks::function<void(), StorageSize, StorageAlignment>;

private:
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

    /**
     * @brief Comparison functor for ordering event nodes in the scheduled queue.
     * @note Nodes with earlier due times are considered "smaller" and will be prioritized in the min-heap.
     * @tparam lhs Left-hand side event node.
     * @tparam rhs Right-hand side event node.
     * @return `true` if `lhs` should be ordered before `rhs`, `false` otherwise.
     */
    struct event_node_compare
    {
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

    /**
     * @brief Wrapper for storing a callable and its bound arguments.
     * @tparam Callable The type of the callable object.
     * @tparam Args The types of the arguments to be bound to the callable.
     * @note The callable and its arguments are stored internally and invoked when the task is executed.
     */
    template <typename Callable, typename... Args>
    class bound_task
    {
    public:
        using callable_type = meta::decay_t<Callable>;
        using args_tuple_type = castle::tuple<meta::decay_t<Args>...>;

        /**
         * @brief Constructs a bound task by storing the callable and its arguments.
         * @param callable The callable object to be stored.
         * @param args The arguments to be bound to the callable.
         * @note The callable and its arguments are stored internally and invoked when the task is executed.
         */
        bound_task(Callable&& callable, Args&&... args)
            : callable_(CASTLE_FORWARD<Callable>(callable))
            , args_(CASTLE_FORWARD<Args>(args)...)
        {
        }

        /**
         * @brief Invokes the stored callable with its bound arguments.
         */
        void operator()()
        {
            invoke(castle::sequence::index_sequence_for<Args...>{});
        }

    private:
        /**
         * @brief Invokes the stored callable with its bound arguments.
         * @tparam Indices The indices of the arguments in the tuple.
         * @note This function is called internally by `operator()()`.
         */
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
        using args_tuple_type = castle::tuple<meta::decay_t<Args>...>;

        /**
         * @brief Constructs a member bound task by storing the object, member function, and its arguments.
         * @param object The object on which the member function will be called.
         * @param member The member function to be stored.
         */
        member_bound_task(T* object, member_type member, Args&&... args)
            : object_(object)
            , member_(member)
            , args_(CASTLE_FORWARD<Args>(args)...)
        {
        }

        /**
         * @brief Invokes the stored member function on the object with its bound arguments.
         * @note The member function is called internally by `operator()()`.
         * @param args The arguments to be bound to the member function.
         */
        void operator()()
        {
            invoke(castle::sequence::index_sequence_for<Args...>{});
        }

    private:
        /**
         * @brief Invokes the stored member function on the object with its bound arguments.
         * @tparam Indices The indices of the arguments in the tuple.
         * @note This function is called internally by `operator()()`.
         * @param args The arguments to be bound to the member function.
         */
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
    
    
    
    /**
     * @brief Starts the worker thread and prepares every event slot.
     * @note If initialization fails, `initialized()` returns `false` and posting reports `status_type::not_configured`.
     */
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

        // Publish the running flag before thread creation to avoid a startup race.
        running_ = true;

        CASTLE_CONST int create_result = pthread_create(&thread_, nullptr, &event_handler::thread_entry, this);

        if (create_result != 0)
        {
            running_ = false;
            destroy_synchronization();
            return;
        }

        initialized_ = true;
        thread_created_ = true;
    }

    /**
     * @brief Copy construction is disabled because the handler owns a worker thread and slot storage.
     */
    event_handler(CASTLE_CONST event_handler&) CASTLE_DELETE;
    /**
     * @brief Copy assignment is disabled because the handler owns a worker thread and slot storage.
     */
    event_handler& operator=(CASTLE_CONST event_handler&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled because the handler owns a worker thread and slot storage.
     */
    event_handler(event_handler&&) CASTLE_DELETE;
    /**
     * @brief Move assignment is disabled because the handler owns a worker thread and slot storage.
     */
    event_handler& operator=(event_handler&&) CASTLE_DELETE;

    /**
     * @brief Shuts down the worker thread and discards any pending events.
     * @note Destruction is equivalent to calling `shutdown()` and ignoring the returned status.
     */
    ~event_handler()
    {
        static_cast<void>(shutdown());
    }

    /**
     * @brief Returns the total number of event slots compiled into the handler.
     * @return `MaxEventCount`.
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return MaxEventCount;
    }

    /**
     * @brief Returns the inline callback storage size for each event slot.
     * @return `StorageSize` in bytes.
     */
    static CASTLE_CONSTEXPR size_type callback_storage_size() CASTLE_NOEXCEPT
    {
        return StorageSize;
    }

    /**
     * @brief Returns the inline callback storage alignment for each event slot.
     * @return `StorageAlignment` in bytes.
     */
    static CASTLE_CONSTEXPR size_type callback_storage_alignment() CASTLE_NOEXCEPT
    {
        return StorageAlignment;
    }

    /**
     * @brief Reports whether the worker thread and synchronization primitives were created successfully.
     * @return `true` when the handler is ready to accept events; otherwise `false`.
     */
    bool initialized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return initialized_;
    }

    /**
     * @brief Reports whether the handler is currently marked as running.
     * @return `true` while the worker loop is active; otherwise `false`.
     * @note Returns `false` when the state cannot be queried because synchronization is unavailable.
     */
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

    /**
     * @brief Returns the number of events currently scheduled in the timing queue.
     * @return Count of queued events that have not yet been popped for execution.
     */
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

    /**
     * @brief Returns the number of free event slots available for new postings.
     * @return Free slot count.
     */
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

    /**
     * @brief Posts one callable for immediate asynchronous execution.
     * @tparam Callable Callable type stored in the handler slot.
     * @param callable Callable object to execute on the worker thread.
     * @return `status_type::ok` on success; otherwise an error describing why the event could not be queued.
     * @warning The callable must fit within the configured inline callback storage.
     */
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

    /**
     * @brief Posts one callable plus bound arguments for immediate asynchronous execution.
     * @tparam Callable Callable type stored in the handler slot.
     * @tparam Args Argument types copied or moved into the bound task.
     * @param callable Callable object to execute on the worker thread.
     * @param args Arguments bound into the stored task.
     * @return `status_type::ok` on success; otherwise an error describing why the event could not be queued.
     * @warning The bound callable and all stored arguments together must fit within the configured inline callback storage.
     */
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

    /**
     * @brief Posts one callable for delayed asynchronous execution.
     * @tparam Callable Callable type stored in the handler slot.
     * @param delay_ms Delay, in milliseconds, measured from the current steady-clock time.
     * @param callable Callable object to execute on the worker thread.
     * @return `status_type::ok` on success; otherwise an error describing why the event could not be queued.
     */
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
    
    /**
     * @brief Posts one callable plus bound arguments for delayed asynchronous execution.
     * @tparam Callable Callable type stored in the handler slot.
     * @tparam Args Argument types copied or moved into the bound task.
     * @param delay_ms Delay, in milliseconds, measured from the current steady-clock time.
     * @param callable Callable object to execute on the worker thread.
     * @param args Arguments bound into the stored task.
     * @return `status_type::ok` on success; otherwise an error describing why the event could not be queued.
     */
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

    /**
     * @brief Posts one callable for repeated asynchronous execution.
     * @tparam Callable Callable type stored in the handler slot.
     * @param times Total number of executions to schedule.
     * @param duration_ms Milliseconds between scheduled executions.
     * @param callable Callable object to execute on the worker thread.
     * @return `status_type::ok` on success or when `times == 0`; otherwise an error describing why the event could not be queued.
     * @note The first execution is scheduled immediately.
     */
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

    /**
     * @brief Posts one callable plus bound arguments for repeated asynchronous execution.
     * @tparam Callable Callable type stored in the handler slot.
     * @tparam Args Argument types copied or moved into the bound task.
     * @param times Total number of executions to schedule.
     * @param duration_ms Milliseconds between scheduled executions.
     * @param callable Callable object to execute on the worker thread.
     * @param args Arguments bound into the stored task.
     * @return `status_type::ok` on success or when `times == 0`; otherwise an error describing why the event could not be queued.
     * @note The first execution is scheduled immediately.
     */
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

    /**
     * @brief Posts one non-const member function for immediate execution.
     * @tparam T Object type that owns the member function.
     * @tparam Args Argument types copied or moved into the bound task.
     * @param member Member function pointer to invoke.
     * @param object Target object pointer.
     * @param args Arguments forwarded to the member function.
     * @return Result of the corresponding object-first overload.
     * @warning `object` must remain alive until the callback finishes executing.
     */
    template <typename T, typename... Args>
    status_type post_event(
        void (T::*member)(Args...),
        T* object,
        Args&&... args) CASTLE_NOEXCEPT
    {
        return post_event(object, member, CASTLE_FORWARD<Args>(args)...);
    }

    /**
     * @brief Posts one non-const member function for delayed execution.
     * @tparam T Object type that owns the member function.
     * @tparam Args Argument types copied or moved into the bound task.
     * @param delay_ms Delay, in milliseconds, measured from the current steady-clock time.
     * @param member Member function pointer to invoke.
     * @param object Target object pointer.
     * @param args Arguments forwarded to the member function.
     * @return Result of the corresponding object-first overload.
     * @warning `object` must remain alive until the callback finishes executing.
     */
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

    /**
     * @brief Posts one non-const member function for repeated execution.
     * @tparam T Object type that owns the member function.
     * @tparam Args Argument types copied or moved into the bound task.
     * @param times Total number of executions to schedule.
     * @param duration_ms Milliseconds between scheduled executions.
     * @param member Member function pointer to invoke.
     * @param object Target object pointer.
     * @param args Arguments forwarded to the member function.
     * @return Result of the corresponding object-first overload.
     * @warning `object` must remain alive until the callback finishes executing.
     */
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

    /**
     * @brief Posts one non-const member function for immediate execution.
     * @tparam T Object type that owns the member function.
     * @tparam Args Argument types copied or moved into the bound task.
     * @param object Target object pointer.
     * @param member Member function pointer to invoke.
     * @param args Arguments forwarded to the member function.
     * @return `status_type::ok` on success, `status_type::invalid_argument` when `object` or `member` is null, or another status describing a queueing failure.
     * @warning `object` must remain alive until the callback finishes executing.
     */
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

    /**
     * @brief Posts one non-const member function for delayed execution.
     * @tparam T Object type that owns the member function.
     * @tparam Args Argument types copied or moved into the bound task.
     * @param delay_ms Delay, in milliseconds, measured from the current steady-clock time.
     * @param object Target object pointer.
     * @param member Member function pointer to invoke.
     * @param args Arguments forwarded to the member function.
     * @return `status_type::ok` on success, `status_type::invalid_argument` when `object` or `member` is null, or another status describing a queueing failure.
     * @warning `object` must remain alive until the callback finishes executing.
     */
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

    /**
     * @brief Posts one non-const member function for repeated execution.
     * @tparam T Object type that owns the member function.
     * @tparam Args Argument types copied or moved into the bound task.
     * @param times Total number of executions to schedule.
     * @param duration_ms Milliseconds between scheduled executions.
     * @param object Target object pointer.
     * @param member Member function pointer to invoke.
     * @param args Arguments forwarded to the member function.
     * @return `status_type::ok` on success, `status_type::ok` when `times == 0`, `status_type::invalid_argument` when `object` or `member` is null, or another status describing a queueing failure.
     * @note The first execution is scheduled immediately when `times > 0`.
     * @warning `object` must remain alive until the callback finishes executing.
     */
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

    /**
     * @brief Stops the worker thread and discards every pending event.
     * @return `status_type::ok` on successful shutdown or when already stopped; otherwise `status_type::system_call_error`.
     * @note Shutdown is idempotent.
     * @warning Do not call this function from a callback executing on the handler's own worker thread.
     */
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

        // Join before releasing slot storage so an in-flight callback cannot access destroyed state.
        CASTLE_CONST int broadcast_result = pthread_cond_broadcast(&condition_);
        pthread_mutex_unlock(&mutex_);

        if (pthread_equal(pthread_self(), thread_) != 0)
        {
            return status_type::system_call_error;
        }

        CASTLE_CONST int join_result = pthread_join(thread_, nullptr);

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

    /**
     * @brief Alias for `shutdown()`.
     * @return Result of `shutdown()`.
     */
    status_type stop() CASTLE_NOEXCEPT
    {
        return shutdown();
    }

    /**
     * @brief Alias for `running()`.
     * @return Result of `running()`.
     */
    bool is_running() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return running();
    }

private:
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
                continue;
            }

            scheduled_queue_.pop();
            size_type slot = top.slot;
            event_slot& current = slots_[slot];
            task_type* task = &current.task;
            pthread_mutex_unlock(&mutex_);

            // Execute outside the mutex so callbacks can post additional work without deadlocking.
            (*task)(); 

            if (pthread_mutex_lock(&mutex_) != 0)
            {
                return;
            }

            if (!current.occupied)
            {
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
            }

            // A failed requeue drops the repeating event instead of corrupting internal state.
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

} 
} 

#else

#error "castle::events::event_handler requires CASTLE_USING_PTHREAD."

#endif 
#endif 
