#include <gtest/gtest.h>
#include "castle/sync/mutex.hpp"

#include <atomic>
#include <thread>

namespace
{
TEST(MutexTest, LockTryLockUnlock)
{
    castle::mutex m;
    static_assert(noexcept(m.lock()),"lock");
    static_assert(noexcept(m.try_lock()),"try");
    EXPECT_TRUE(m.try_lock());
    EXPECT_FALSE(m.try_lock());
    m.unlock();
    m.lock();
    EXPECT_FALSE(m.try_lock());
    m.unlock();
    EXPECT_TRUE(m.try_lock());
    m.unlock();
}


// ---------------------------------------------------------------------------
// castle::mutex defaults to castle::spin_wait, castle's zero-dependency,
// bare-metal-safe WaitPolicy.
// ---------------------------------------------------------------------------
TEST(MutexTest, WaitPolicyDefaultsToSpinWait)
{
    static_assert(castle::is_same<castle::mutex::wait_policy_type, castle::spin_wait>::value,
                  "default WaitPolicy must be castle::spin_wait");
}


// ---------------------------------------------------------------------------
// A custom WaitPolicy is invoked on each failed retry of a blocked lock(),
// instead of castle::spin_wait's default CPU-relax hint. This is the
// extension point an RTOS integration would use to yield the calling task
// to a real scheduler rather than spin.
// ---------------------------------------------------------------------------
struct counting_wait_policy
{
    static std::atomic<uint32_t> hit_count;

    static void wait() noexcept
    {
        hit_count.fetch_add(1U, std::memory_order_relaxed);
    }
};

std::atomic<uint32_t> counting_wait_policy::hit_count{0U};

TEST(MutexTest, CustomWaitPolicyInvokedWhileBlocked)
{
    counting_wait_policy::hit_count.store(0U);

    castle::basic_mutex<counting_wait_policy> m;

    static_assert(castle::is_same<decltype(m)::wait_policy_type, counting_wait_policy>::value,
                  "wait_policy_type");

    m.lock();

    bool waiter_started = false;
    bool waiter_finished = false;

    std::thread waiter(
        [&m, &waiter_started, &waiter_finished]()
        {
            waiter_started = true;

            m.lock();

            waiter_finished = true;
        });

    while (!waiter_started)
    {
        std::this_thread::yield();
    }

    while (counting_wait_policy::hit_count.load() == 0U)
    {
        std::this_thread::yield();
    }

    EXPECT_FALSE(waiter_finished);
    EXPECT_GT(counting_wait_policy::hit_count.load(), 0U);

    m.unlock();

    waiter.join();

    EXPECT_TRUE(waiter_finished);

    m.unlock();
}
}
