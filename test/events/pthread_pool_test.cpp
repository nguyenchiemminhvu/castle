#ifndef CASTLE_USING_PTHREAD
#define CASTLE_USING_PTHREAD 1
#endif // CASTLE_USING_PTHREAD

#include "castle/events/pthread_pool.hpp"

#include <gtest/gtest.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <atomic>
#include <time.h>

extern "C"
{
    int __real_pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*);
    int __real_pthread_mutex_destroy(pthread_mutex_t*);
    int __real_pthread_mutex_lock(pthread_mutex_t*);
    int __real_pthread_mutex_unlock(pthread_mutex_t*);
    int __real_pthread_cond_init(pthread_cond_t*, const pthread_condattr_t*);
    int __real_pthread_cond_destroy(pthread_cond_t*);
    int __real_pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*);
    int __real_pthread_cond_signal(pthread_cond_t*);
    int __real_pthread_cond_broadcast(pthread_cond_t*);
    int __real_pthread_create(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
    int __real_pthread_join(pthread_t, void**);
}

namespace test_control
{

static std::atomic<int> mutex_init_failure;
static std::atomic<int> mutex_lock_failure;
static std::atomic<int> cond_init_failure;
static std::atomic<int> cond_wait_failure;
static std::atomic<int> cond_signal_failure;
static std::atomic<int> cond_broadcast_failure;
static std::atomic<int> pthread_create_failure;
static std::atomic<int> pthread_join_failure;

static void reset()
{
    atomic_store_explicit(&mutex_init_failure, 0, std::memory_order_seq_cst);
    atomic_store_explicit(&mutex_lock_failure, 0, std::memory_order_seq_cst);
    atomic_store_explicit(&cond_init_failure, 0, std::memory_order_seq_cst);
    atomic_store_explicit(&cond_wait_failure, 0, std::memory_order_seq_cst);
    atomic_store_explicit(&cond_signal_failure, 0, std::memory_order_seq_cst);
    atomic_store_explicit(&cond_broadcast_failure, 0, std::memory_order_seq_cst);
    atomic_store_explicit(&pthread_create_failure, 0, std::memory_order_seq_cst);
    atomic_store_explicit(&pthread_join_failure, 0, std::memory_order_seq_cst);
}

static bool consume(std::atomic<int>* value)
{
    int expected = 1;
    return value->compare_exchange_strong(expected, 0);
}

} // namespace test_control

extern "C" int __wrap_pthread_mutex_init(
    pthread_mutex_t* mutex,
    const pthread_mutexattr_t* attribute)
{
    if (test_control::consume(&test_control::mutex_init_failure))
    {
        return EAGAIN;
    }

    return __real_pthread_mutex_init(mutex, attribute);
}

extern "C" int __wrap_pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    return __real_pthread_mutex_destroy(mutex);
}

extern "C" int __wrap_pthread_mutex_lock(pthread_mutex_t* mutex)
{
    if (test_control::consume(&test_control::mutex_lock_failure))
    {
        return EBUSY;
    }

    return __real_pthread_mutex_lock(mutex);
}

extern "C" int __wrap_pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    return __real_pthread_mutex_unlock(mutex);
}

extern "C" int __wrap_pthread_cond_init(
    pthread_cond_t* condition,
    const pthread_condattr_t* attribute)
{
    if (test_control::consume(&test_control::cond_init_failure))
    {
        return EAGAIN;
    }

    return __real_pthread_cond_init(condition, attribute);
}

extern "C" int __wrap_pthread_cond_destroy(pthread_cond_t* condition)
{
    return __real_pthread_cond_destroy(condition);
}

extern "C" int __wrap_pthread_cond_wait(
    pthread_cond_t* condition,
    pthread_mutex_t* mutex)
{
    if (test_control::consume(&test_control::cond_wait_failure))
    {
        return EINVAL;
    }

    return __real_pthread_cond_wait(condition, mutex);
}

extern "C" int __wrap_pthread_cond_signal(pthread_cond_t* condition)
{
    if (test_control::consume(&test_control::cond_signal_failure))
    {
        __real_pthread_cond_signal(condition);
        return EIO;
    }

    return __real_pthread_cond_signal(condition);
}

extern "C" int __wrap_pthread_cond_broadcast(pthread_cond_t* condition)
{
    if (test_control::consume(&test_control::cond_broadcast_failure))
    {
        __real_pthread_cond_broadcast(condition);
        return EIO;
    }

    return __real_pthread_cond_broadcast(condition);
}

extern "C" int __wrap_pthread_create(
    pthread_t* thread,
    const pthread_attr_t* attribute,
    void* (*entry)(void*),
    void* argument)
{
    if (test_control::consume(&test_control::pthread_create_failure))
    {
        return EAGAIN;
    }

    return __real_pthread_create(thread, attribute, entry, argument);
}

extern "C" int __wrap_pthread_join(pthread_t thread, void** value)
{
    if (test_control::consume(&test_control::pthread_join_failure))
    {
        __real_pthread_join(thread, value);
        return EINVAL;
    }

    return __real_pthread_join(thread, value);
}

namespace
{

using pool_type = castle::events::pthread_pool<2U, 2U>;

class PthreadPoolTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_control::reset();
    }

    void TearDown() override
    {
        test_control::reset();
    }
};

static void sleep_milliseconds(long milliseconds)
{
    timespec timeout;
    timeout.tv_sec = milliseconds / 1000L;
    timeout.tv_nsec = (milliseconds % 1000L) * 1000000L;
    nanosleep(&timeout, nullptr);
}

static bool wait_for_value(
    std::atomic<int>* value,
    int expected,
    long timeout_milliseconds)
{
    for (long elapsed = 0L; elapsed < timeout_milliseconds; elapsed += 1L)
    {
        if (value->load(std::memory_order_seq_cst) == expected)
        {
            return true;
        }

        sleep_milliseconds(1L);
    }

    return value->load(std::memory_order_seq_cst) == expected;
}

TEST(PthreadPoolCompileTimeTest, Configuration)
{
    static_assert(pool_type::thread_count() == 2U, "wrong thread count");
    static_assert(pool_type::task_capacity() == 2U, "wrong task capacity");
}

TEST_F(PthreadPoolTest, InitialStateAndEmptyTask)
{
    test_control::reset();
    pool_type pool;

    EXPECT_TRUE(pool.running());
    EXPECT_EQ(0U, pool.queued());
    EXPECT_EQ(2U, pool.available());
    EXPECT_FALSE(pool.submit(pool_type::task_type()));
    EXPECT_TRUE(pool.running());
    EXPECT_TRUE(pool.stop());
    EXPECT_FALSE(pool.running());
    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, CallableExecutionAndSlotReuse)
{
    test_control::reset();
    pool_type pool;
    std::atomic<int> completed{0};

    assert(pool.submit([&completed]() {
        completed.fetch_add(1, std::memory_order_seq_cst);
    }));

    assert(pool.submit([&completed]() {
        completed.fetch_add(1, std::memory_order_seq_cst);
    }));

    EXPECT_TRUE(wait_for_value(&completed, 2, 1000L));
    EXPECT_EQ(0U, pool.queued());
    EXPECT_EQ(2U, pool.available());

    assert(pool.submit([&completed]() {
        completed.fetch_add(1, std::memory_order_seq_cst);
    }));
    EXPECT_TRUE(wait_for_value(&completed, 3, 1000L));
    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, PendingCapacity)
{
    test_control::reset();
    pool_type pool;

    /*
    * Fill the pool with two callbacks that wait on an application-owned flag.
    * Both worker threads remain occupied, leaving the pending queue available.
    */
    std::atomic<int> release_tasks{0};

    struct TaskContext
    {
        std::atomic<int>* release;
    };

    TaskContext task_context = {&release_tasks};

    assert(pool.submit([task_context]() {
        while (task_context.release->load(std::memory_order_seq_cst) == 0)
        {
            sched_yield();
        }
    }));

    assert(pool.submit([task_context]() {
        while (task_context.release->load(std::memory_order_seq_cst) == 0)
        {
            sched_yield();
        }
    }));

    while (pool.available() != 2U)
    {
        sched_yield();
    }

    EXPECT_TRUE(pool.submit([]() {}));
    EXPECT_TRUE(pool.submit([]() {}));
    EXPECT_FALSE(pool.submit([]() {}));
    EXPECT_FALSE(pool.submit([]() {}));
    EXPECT_EQ(2U, pool.queued());

    atomic_store_explicit(&release_tasks, 1, std::memory_order_seq_cst);
    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, TaskTypeSubmission)
{
    test_control::reset();
    pool_type pool;
    std::atomic<int> completed{0};

    pool_type::task_type task([&completed]() {
        completed.fetch_add(1, std::memory_order_seq_cst);
    });

    EXPECT_TRUE(pool.submit(static_cast<pool_type::task_type&&>(task)));
    EXPECT_TRUE(wait_for_value(&completed, 1, 1000L));
    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, SubmitMutexFailure)
{
    test_control::reset();
    pool_type pool;

    atomic_store_explicit(&test_control::mutex_lock_failure, 1, std::memory_order_seq_cst);
    EXPECT_FALSE(pool.submit([]() {}));

    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, SubmitSignalFailure)
{
    test_control::reset();
    pool_type pool;

    atomic_store_explicit(&test_control::cond_signal_failure, 1, std::memory_order_seq_cst);
    EXPECT_FALSE(pool.submit([]() {}));
    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, StatusMutexFailures)
{
    test_control::reset();
    pool_type pool;

    atomic_store_explicit(&test_control::mutex_lock_failure, 1, std::memory_order_seq_cst);
    EXPECT_FALSE(pool.running());

    atomic_store_explicit(&test_control::mutex_lock_failure, 1, std::memory_order_seq_cst);
    EXPECT_EQ(0U, pool.queued());

    atomic_store_explicit(&test_control::mutex_lock_failure, 1, std::memory_order_seq_cst);
    EXPECT_EQ(0U, pool.available());

    EXPECT_TRUE(pool.stop());
}


TEST_F(PthreadPoolTest, StopMutexFailure)
{
    test_control::reset();
    pool_type pool;

    atomic_store_explicit(&test_control::mutex_lock_failure, 1, std::memory_order_seq_cst);
    EXPECT_FALSE(pool.stop());

    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, StopBroadcastFailure)
{
    test_control::reset();
    pool_type pool;

    atomic_store_explicit(&test_control::cond_broadcast_failure, 1, std::memory_order_seq_cst);
    EXPECT_FALSE(pool.stop());
    EXPECT_FALSE(pool.running());
}

TEST_F(PthreadPoolTest, StopJoinFailure)
{
    test_control::reset();
    pool_type pool;

    atomic_store_explicit(&test_control::pthread_join_failure, 1, std::memory_order_seq_cst);
    EXPECT_FALSE(pool.stop());
    EXPECT_FALSE(pool.running());
}


TEST_F(PthreadPoolTest, WorkerMutexLockFailure)
{
    test_control::reset();
    pool_type pool;
    std::atomic<int> completed{0};

    assert(pool.submit([&completed]() {
        atomic_store_explicit(&test_control::mutex_lock_failure, 1, std::memory_order_seq_cst);
        completed.fetch_add(1, std::memory_order_seq_cst);
    }));

    EXPECT_TRUE(wait_for_value(&completed, 1, 1000L));
    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, WorkerCondWaitFailure)
{
    test_control::reset();
    atomic_store_explicit(&test_control::cond_wait_failure, 1, std::memory_order_seq_cst);

    pool_type pool;
    sleep_milliseconds(10L);

    /*
    * The worker exits after the injected condition-wait failure. The pool still
    * remains destructible and stop() can join the terminated worker.
    */
    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, ConstructorMutexFailure)
{
    test_control::reset();
    atomic_store_explicit(&test_control::mutex_init_failure, 1, std::memory_order_seq_cst);

    pool_type pool;
    EXPECT_FALSE(pool.running());
    EXPECT_EQ(0U, pool.queued());
    EXPECT_EQ(0U, pool.available());
    EXPECT_FALSE(pool.submit([]() {}));
    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, ConstructorConditionFailure)
{
    test_control::reset();
    atomic_store_explicit(&test_control::cond_init_failure, 1, std::memory_order_seq_cst);

    pool_type pool;
    EXPECT_FALSE(pool.running());
    EXPECT_EQ(0U, pool.queued());
    EXPECT_EQ(0U, pool.available());
    EXPECT_TRUE(pool.stop());
}


TEST_F(PthreadPoolTest, ShutdownLockFailureAfterThreadCreateFailure)
{
    test_control::reset();
    atomic_store_explicit(&test_control::pthread_create_failure, 1, std::memory_order_seq_cst);
    atomic_store_explicit(&test_control::mutex_lock_failure, 1, std::memory_order_seq_cst);

    pool_type pool;
    EXPECT_FALSE(pool.running());
    EXPECT_TRUE(pool.stop());
}

TEST_F(PthreadPoolTest, ConstructorThreadCreateFailure)
{
    test_control::reset();
    atomic_store_explicit(&test_control::pthread_create_failure, 1, std::memory_order_seq_cst);

    pool_type pool;
    EXPECT_FALSE(pool.running());
    EXPECT_TRUE(pool.stop());
}

} // namespace
