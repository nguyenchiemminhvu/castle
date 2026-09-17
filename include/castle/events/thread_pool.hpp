// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-size pthread worker pool with bounded task storage.
 *
 * Use this component when a POSIX target needs a small, pre-sized pool of
 * worker threads that execute void() tasks without Castle-managed heap
 * allocation. All Castle-owned storage is fixed at compile time: worker
 * handles, task slots, and queue state are embedded directly in the object.
 *
 * @note The pool uses pthread synchronization and is available only when
 * CASTLE_USING_PTHREAD is enabled.
 * @warning stop() prevents workers from taking additional queued tasks. Tasks
 * already executing may finish, but tasks still waiting in the queue are left
 * unexecuted.
 *
 * @code
 * #include "castle/events/thread_pool.hpp"
 *
 * int main()
 * {
 *     castle::events::thread_pool<1U, 2U> pool;
 *     volatile int finished = 0;
 *
 *     pool.submit([&finished]() { ++finished; });
 *
 *     while (finished != 1)
 *     {
 *     }
 *
 *     pool.stop();
 *     return 0;
 * }
 * @endcode
 */
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

/**
 * @brief Bounded pthread-backed thread pool.
 *
 * @tparam ThreadCount Number of worker threads created during construction.
 * @tparam PendingTaskCount Number of task slots available for submitted work.
 * @tparam StorageSize Inline storage reserved for each submitted callable.
 * @tparam StorageAlignment Alignment of each task slot's inline storage.
 *
 * @note submit(), stop(), running(), queued(), and available() synchronize on
 * an internal pthread mutex.
 */
template <size_type ThreadCount
        , size_type PendingTaskCount
        , size_type StorageSize = castle::inplace_storage_reserved
        , size_type StorageAlignment = castle::inplace_alignment_default>
class thread_pool
{
    static_assert(ThreadCount > 0U, "thread_pool thread count must be non-zero");
    static_assert(PendingTaskCount > 0U, "thread_pool task count must be non-zero");

public:
    /** @brief Type of task stored by the pool. */
    using task_type = castle::callbacks::function<void(), StorageSize, StorageAlignment>;

    /**
     * @brief Construct the pool and start worker threads immediately.
     *
     * @note Construction is best-effort because the type is noexcept. If mutex,
     * condition variable, or thread creation fails, the object remains valid
     * but may report running() == false and reject submissions.
     */
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

    /** @brief Copy and move are disabled because worker threads keep the pool address. */
    thread_pool(CASTLE_CONST thread_pool&) CASTLE_DELETE;
    thread_pool& operator=(CASTLE_CONST thread_pool&) CASTLE_DELETE;

    thread_pool(thread_pool&&) CASTLE_DELETE;
    thread_pool& operator=(thread_pool&&) CASTLE_DELETE;

    /**
     * @brief Stop the pool if needed, then release synchronization resources.
     */
    ~thread_pool()
    {
        static_cast<void>(stop());
        destroy_synchronization();
    }

    /**
     * @brief Submit a pre-built task object.
     *
     * @param task Task to move into an available task slot.
     * @return true when the task is accepted, otherwise false.
     *
     * @note A task is rejected when the callback is empty, synchronization is
     * unavailable, stop() has been requested, no free slot exists, the pending
     * queue is full, or a pthread operation fails.
     * @warning Successful submission only means the task was queued. If stop()
     * is called before a worker pops that task, the task may never execute.
     */
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
            tasks_[task_index] = task_type();
            free_queue_.push(task_index);
            pthread_mutex_unlock(&queue_mutex_);
            return false;
        }

        CASTLE_CONST int signal_result = pthread_cond_signal(&queue_condition_);

        pthread_mutex_unlock(&queue_mutex_);

        return signal_result == 0;
    }

    /**
     * @brief Submit any callable convertible to task_type.
     *
     * @tparam Callable Callable type invocable as void().
     * @param callable Callable to store by value in the queue.
     * @return true when the task is accepted, otherwise false.
     */
    template <typename Callable>
    CASTLE_NODISCARD bool submit(Callable&& callable) CASTLE_NOEXCEPT
    {
        using callable_type = meta::decay_t<Callable>;

        static_assert(meta::is_same<meta::invoke_result_t<callable_type&>, void>::value,
                      "thread_pool task callable must return void and take no arguments"
        );

        return submit(task_type(CASTLE_FORWARD<Callable>(callable)));
    }

    /**
     * @brief Request shutdown and join every created worker thread.
     *
     * @return true when shutdown signaling and joins succeed, otherwise false.
     *
     * @note New submissions are rejected after stop_requested_ becomes true.
     * @note Workers already executing a task may finish that task before
     * joining completes.
     * @warning Workers exit as soon as they observe stop_requested_, even if
     * pending_queue_ still contains tasks.
     */
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

    /**
     * @brief Report whether the pool is still accepting new work.
     *
     * @return true when initialization succeeded and stop() has not been
     * requested, otherwise false.
     */
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

    /**
     * @brief Count tasks still waiting in the pending queue.
     *
     * @return The current queue depth, or zero when synchronization is
     * unavailable.
     *
     * @note Tasks already moved into a worker-local variable are not counted.
     */
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

    /**
     * @brief Count currently free task slots.
     *
     * @return The number of reusable task slots, or zero when synchronization
     * is unavailable.
     *
     * @note A slot returns to the free queue immediately after a worker moves
     * the task into a local variable, before the callable starts running.
     */
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

    /**
     * @brief Query the compile-time worker count.
     *
     * @return ThreadCount.
     */
    static CASTLE_CONSTEXPR size_type thread_count() CASTLE_NOEXCEPT
    {
        return ThreadCount;
    }

    /**
     * @brief Query the compile-time task-slot capacity.
     *
     * @return PendingTaskCount.
     */
    static CASTLE_CONSTEXPR size_type task_capacity() CASTLE_NOEXCEPT
    {
        return PendingTaskCount;
    }

private:
    /**
     * @brief Initialize the synchronization primitives (mutex and condition variable) used by the thread pool.
     * @return True if the synchronization primitives were successfully initialized, false otherwise.
     */
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

    /**
     * @brief Destroy the synchronization primitives (mutex and condition variable) used by the thread pool.
     * @note This function should be called when the thread pool is being shut down to ensure proper cleanup of synchronization resources.
     */
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

    /**
     * @brief Shut down all threads that have been created by the thread pool.
     * @note This function should be called when the thread pool is being shut down to ensure all threads are properly terminated and resources are released.
     */
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

    /**
     * @brief Entry point for worker threads in the thread pool.
     * @note This function is intended to be used internally by the thread pool and should not be called directly.
     */
    static void* worker_entry(void* arg) CASTLE_NOEXCEPT
    {
        thread_pool* pool = static_cast<thread_pool*>(arg);
        pool->worker_loop();
        return nullptr;
    }

    /**
     * @brief Main loop executed by worker threads to process tasks from the thread pool's queue.
     * @note This function is intended to be used internally by the thread pool and should not be called directly.
     */
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

            // Recycle the slot before running user code so queue capacity is
            // bounded only by pending tasks, not by tasks currently executing.
            free_queue_.push(task_index);

            pthread_mutex_unlock(&queue_mutex_);

            // Run outside the mutex so one task cannot block queue progress.
            task();
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
