#include <gtest/gtest.h>

#include "castle/sync/recursive_mutex.hpp"

#include <atomic>
#include <thread>

namespace
{

// ---------------------------------------------------------------------------
// The owning thread can re-acquire the lock, and it stays held until the
// matching number of unlock() calls has been made.
// ---------------------------------------------------------------------------
TEST(RecursiveMutexTest, SameThreadReacquires)
{
    castle::recursive_mutex m;

    static_assert(noexcept(m.lock()), "lock");
    static_assert(noexcept(m.try_lock()), "try_lock");
    static_assert(noexcept(m.unlock()), "unlock");

    m.lock();
    EXPECT_TRUE(m.try_lock());
    m.lock();

    m.unlock();
    m.unlock();
    m.unlock();

    EXPECT_TRUE(m.try_lock());
    m.unlock();
}


// ---------------------------------------------------------------------------
// A different thread cannot acquire the lock until every recursive
// acquisition has a matching unlock().
//
// try_lock() is used from the other thread so the test stays deterministic
// (no blocking, no timing assumptions).
// ---------------------------------------------------------------------------
TEST(RecursiveMutexTest, OtherThreadWaitsForLastUnlock)
{
    castle::recursive_mutex m;

    m.lock();
    m.lock();

    std::thread still_locked_twice([&m]() { EXPECT_FALSE(m.try_lock()); });
    still_locked_twice.join();

    m.unlock();

    std::thread still_locked_once([&m]() { EXPECT_FALSE(m.try_lock()); });
    still_locked_once.join();

    m.unlock();

    std::thread now_free(
        [&m]()
        {
            EXPECT_TRUE(m.try_lock());
            m.unlock();
        });
    now_free.join();
}


// ---------------------------------------------------------------------------
// castle::recursive_mutex defaults to castle::spin_wait, castle's
// zero-dependency, bare-metal-safe WaitPolicy.
// ---------------------------------------------------------------------------
TEST(RecursiveMutexTest, WaitPolicyDefaultsToSpinWait)
{
    static_assert(castle::is_same<castle::recursive_mutex::wait_policy_type, castle::spin_wait>::value,
                  "default WaitPolicy must be castle::spin_wait");
}


// ---------------------------------------------------------------------------
// A custom WaitPolicy is invoked on each failed retry of a blocked lock(),
// instead of castle::spin_wait's default CPU-relax hint.
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

TEST(RecursiveMutexTest, CustomWaitPolicyInvokedWhileBlocked)
{
    counting_wait_policy::hit_count.store(0U);

    castle::basic_recursive_mutex<counting_wait_policy> m;

    static_assert(castle::is_same<decltype(m)::wait_policy_type, counting_wait_policy>::value,
                  "wait_policy_type");

    bool other_thread_locked = false;
    bool other_thread_blocked_attempted = false;

    std::thread owner(
        [&m, &other_thread_locked, &other_thread_blocked_attempted]()
        {
            m.lock();
            other_thread_locked = true;

            while (!other_thread_blocked_attempted)
            {
                std::this_thread::yield();
            }

            m.unlock();
        });

    while (!other_thread_locked)
    {
        std::this_thread::yield();
    }

    std::thread waiter(
        [&m, &other_thread_blocked_attempted]()
        {
            other_thread_blocked_attempted = true;

            m.lock();
            m.unlock();
        });

    while (counting_wait_policy::hit_count.load() == 0U)
    {
        std::this_thread::yield();
    }

    EXPECT_GT(counting_wait_policy::hit_count.load(), 0U);

    owner.join();
    waiter.join();
}

} // namespace
