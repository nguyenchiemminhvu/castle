#ifndef CASTLE_EVENTS_THREAD_POOL_HPP
#define CASTLE_EVENTS_THREAD_POOL_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/container/ring_buffer.hpp"
#include "castle/callbacks/function.hpp"

#if CASTLE_USING_PTHREAD

#include <pthread.h>

namespace castle
{
namespace events
{

// Fixed-size pthread thread pool. Thread storage and task storage are owned by
// the object and no dynamic allocation is performed by CASTLE.
//
// The pending queue contains task-slot indices instead of function
// objects because castle::container::ring_buffer intentionally requires its
// element type to be trivially copyable and trivially destructible.
template <size_type ThreadCount
        , size_type PendingTaskCount
        , size_type StorageSize = castle::inplace_storage_reserved
        , size_type StorageAlignment = castle::inplace_alignment_default>
class thread_pool
{
    static_assert(ThreadCount > 0U, "thread_pool thread count must be non-zero");
    static_assert(PendingTaskCount > 0U, "thread_pool task count must be non-zero");

public:
    using task_type = castle::callbacks::function<void(), StorageSize, StorageAlignment>;

    thread_pool() CASTLE_NOEXCEPT
        : created_thread_count_(0U)
        , running_(false)
        , stop_requested_(false)
        , mutex_initialized_(false)
        , condition_initialized_(false)
    {
        for (size_type i = 0U; i < PendingTaskCount; ++i)
        {
            if (!free_queue_.push(i))
            {
                CASTLE_ASSERT_FAIL(CASTLE_ERROR_GENERIC("thread_pool failed to initialize free task queue"));
                return;
            }
        }

        if (!initialize_synchronization())
        {
            CASTLE_ASSERT_FAIL(CASTLE_ERROR_GENERIC("thread_pool failed to initialize synchronization"));
            return;
        }

        for (size_type i = 0U; i < ThreadCount; ++i)
        {
            CASTLE_CONST int result = pthread_create(
                &threads_[i],
                nullptr,
                &thread_pool::worker_entry,
                this
            );

            if (result != 0)
            {
                shutdown_created_threads();
                return;
            }

            ++created_thread_count_;
        }

        running_ = true;
    }

    thread_pool(CASTLE_CONST thread_pool&) CASTLE_DELETE;
    thread_pool& operator=(CASTLE_CONST thread_pool&) CASTLE_DELETE;

    thread_pool(thread_pool&&) CASTLE_DELETE;
    thread_pool& operator=(thread_pool&&) CASTLE_DELETE;

    ~thread_pool()
    {
        static_cast<void>(stop());
        destroy_synchronization();
    }

    CASTLE_NODISCARD bool submit(task_type&& task) CASTLE_NOEXCEPT
    {
        if (!task || !mutex_initialized_)
        {
            return false;
        }

        if (pthread_mutex_lock(&queue_mutex_) != 0)
        {
            return false;
        }

        if (stop_requested_ || free_queue_.empty() || pending_queue_.full())
        {
            pthread_mutex_unlock(&queue_mutex_);
            return false;
        }

        size_type task_index = 0U;

        if (!free_queue_.pop(task_index))
        {
            pthread_mutex_unlock(&queue_mutex_);
            return false;
        }

        tasks_[task_index] = CASTLE_MOVE(task);

        if (!pending_queue_.push(task_index))
        {
            // This path is protected by the same mutex and should be unreachable when
            // the free-slot and pending-slot invariants are preserved.
            tasks_[task_index] = task_type();
            free_queue_.push(task_index);
            pthread_mutex_unlock(&queue_mutex_);
            return false;
        }

        CASTLE_CONST int signal_result = pthread_cond_signal(&queue_condition_);

        pthread_mutex_unlock(&queue_mutex_);

        return signal_result == 0;
    }

    template <typename Callable>
    CASTLE_NODISCARD bool submit(Callable&& callable) CASTLE_NOEXCEPT
    {
        using callable_type = meta::decay_t<Callable>;

        static_assert(meta::is_same<meta::invoke_result_t<callable_type&>, void>::value,
                      "thread_pool task callable must return void and take no arguments"
        );

        return submit(task_type(CASTLE_FORWARD<Callable>(callable)));
    }

    CASTLE_NODISCARD bool stop() CASTLE_NOEXCEPT
    {
        if (!mutex_initialized_)
        {
            return true;
        }

        if (!running_ && created_thread_count_ == 0U)
        {
            return true;
        }

        if (pthread_mutex_lock(&queue_mutex_) != 0)
        {
            return false;
        }

        stop_requested_ = true;

        CASTLE_CONST int broadcast_result = pthread_cond_broadcast(&queue_condition_);
        pthread_mutex_unlock(&queue_mutex_);

        bool join_ok = (broadcast_result == 0);

        for (size_type i = 0U; i < created_thread_count_; ++i)
        {
            CASTLE_CONST int join_result = pthread_join(threads_[i], nullptr);

            join_ok = join_ok && (join_result == 0);
        }

        created_thread_count_ = 0U;
        running_ = false;

        return join_ok;
    }

    CASTLE_NODISCARD bool running() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!mutex_initialized_)
        {
            return false;
        }

        if (pthread_mutex_lock(&queue_mutex_) != 0)
        {
            return false;
        }

        CASTLE_CONST bool value = running_ && !stop_requested_;
        pthread_mutex_unlock(&queue_mutex_);
        return value;
    }

    CASTLE_NODISCARD size_type queued() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!mutex_initialized_ || pthread_mutex_lock(&queue_mutex_) != 0)
        {
            return 0U;
        }

        CASTLE_CONST size_type value = pending_queue_.size();
        pthread_mutex_unlock(&queue_mutex_);
        return value;
    }

    CASTLE_NODISCARD size_type available() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!mutex_initialized_ || pthread_mutex_lock(&queue_mutex_) != 0)
        {
            return 0U;
        }

        CASTLE_CONST size_type value = free_queue_.size();
        pthread_mutex_unlock(&queue_mutex_);
        return value;
    }

    static CASTLE_CONSTEXPR size_type thread_count() CASTLE_NOEXCEPT
    {
        return ThreadCount;
    }

    static CASTLE_CONSTEXPR size_type task_capacity() CASTLE_NOEXCEPT
    {
        return PendingTaskCount;
    }

private:

    CASTLE_NODISCARD bool initialize_synchronization() CASTLE_NOEXCEPT
    {
        if (pthread_mutex_init(&queue_mutex_, nullptr) != 0)
        {
            return false;
        }

        mutex_initialized_ = true;

        if (pthread_cond_init(&queue_condition_, nullptr) != 0)
        {
            pthread_mutex_destroy(&queue_mutex_);
            mutex_initialized_ = false;
            return false;
        }

        condition_initialized_ = true;
        return true;
    }

    void destroy_synchronization() CASTLE_NOEXCEPT
    {
        if (condition_initialized_)
        {
            pthread_cond_destroy(&queue_condition_);
            condition_initialized_ = false;
        }

        if (mutex_initialized_)
        {
            pthread_mutex_destroy(&queue_mutex_);
            mutex_initialized_ = false;
        }
    }

    void shutdown_created_threads() CASTLE_NOEXCEPT
    {
        if (!mutex_initialized_ || created_thread_count_ == 0U)
        {
            return;
        }

        if (pthread_mutex_lock(&queue_mutex_) == 0)
        {
            stop_requested_ = true;
            pthread_cond_broadcast(&queue_condition_);
            pthread_mutex_unlock(&queue_mutex_);
        }

        for (size_type i = 0U; i < created_thread_count_; ++i)
        {
            pthread_join(threads_[i], nullptr);
        }

        created_thread_count_ = 0U;
        running_ = false;

        destroy_synchronization();
    }

    static void* worker_entry(void* arg) CASTLE_NOEXCEPT
    {
        thread_pool* pool = static_cast<thread_pool*>(arg);
        pool->worker_loop();
        return nullptr;
    }

    void worker_loop() CASTLE_NOEXCEPT
    {
        while (true)
        {
            if (pthread_mutex_lock(&queue_mutex_) != 0)
            {
                return;
            }

            while (pending_queue_.empty() && !stop_requested_)
            {
                CASTLE_CONST int wait_result =
                pthread_cond_wait(&queue_condition_, &queue_mutex_);

                if (wait_result != 0)
                {
                    pthread_mutex_unlock(&queue_mutex_);
                    return;
                }
            }

            if (stop_requested_)
            {
                pthread_mutex_unlock(&queue_mutex_);
                return;
            }

            if (pending_queue_.empty())
            {
                pthread_mutex_unlock(&queue_mutex_);
                return;
            }

            size_type task_index = 0U;
            pending_queue_.pop(task_index);

            task_type task = CASTLE_MOVE(tasks_[task_index]);

            // The task storage is free as soon as ownership has moved to the local task.
            // This keeps PendingTaskCount equal to the number of tasks waiting in the queue.
            free_queue_.push(task_index);

            pthread_mutex_unlock(&queue_mutex_);

            task(); // Execute the task outside of the mutex lock to avoid blocking other threads.
        }
    }

    pthread_t threads_[ThreadCount];
    task_type tasks_[PendingTaskCount];
    castle::container::ring_buffer<size_type, PendingTaskCount> pending_queue_;
    castle::container::ring_buffer<size_type, PendingTaskCount> free_queue_;
    CASTLE_MUTABLE pthread_mutex_t queue_mutex_;
    pthread_cond_t queue_condition_;
    size_type created_thread_count_;

    bool running_;
    bool stop_requested_;
    bool mutex_initialized_;
    bool condition_initialized_;
};

} // namespace events
} // namespace castle

#else

#error "castle::thread_pool requires CASTLE_USING_PTHREAD."

#endif // CASTLE_USING_PTHREAD
#endif // CASTLE_EVENTS_THREAD_POOL_HPP
