#include <gtest/gtest.h>

#include "castle/sync/semaphore.hpp"

#include <atomic>
#include <stdint.h>
#include <thread>

namespace
{

// ---------------------------------------------------------------------------
// Basic API and noexcept checks.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, NoexceptApi)
{
    castle::semaphore<4> s(4U);

    static_assert(noexcept(s.acquire()), "acquire");
    static_assert(noexcept(s.try_acquire()), "try_acquire");
    static_assert(noexcept(s.release()), "release");
    static_assert(noexcept(s.count()), "count");
    static_assert(noexcept(castle::semaphore<4>::max()), "max");
}


// ---------------------------------------------------------------------------
// castle::semaphore defaults to castle::spin_wait, castle's zero-dependency,
// bare-metal-safe WaitPolicy.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, WaitPolicyDefaultsToSpinWait)
{
    static_assert(castle::is_same<castle::semaphore<4>::wait_policy_type, castle::spin_wait>::value,
                  "default WaitPolicy must be castle::spin_wait");
    static_assert(castle::is_same<castle::binary_semaphore::wait_policy_type, castle::spin_wait>::value,
                  "binary_semaphore's default WaitPolicy must be castle::spin_wait");
}


// ---------------------------------------------------------------------------
// The WaitPolicy type-safety probe accepts only a callable, static,
// noexcept, no-argument wait() function - exactly what castle::semaphore's
// static_assert relies on to give a precise compile error for a bad policy.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, HasStaticWaitTraitDetectsValidAndInvalidPolicies)
{
    struct good_policy
    {
        static void wait() noexcept {}
    };

    struct missing_wait_policy
    {
    };

    struct non_noexcept_wait_policy
    {
        static void wait() {}
    };

    static_assert(castle::detail::has_static_wait<good_policy>::value, "good_policy");
    static_assert(!castle::detail::has_static_wait<missing_wait_policy>::value, "missing_wait_policy");
    static_assert(!castle::detail::has_static_wait<non_noexcept_wait_policy>::value, "non_noexcept_wait_policy");
}


// ---------------------------------------------------------------------------
// Compile-time capacity.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, MaxIsCompileTimeCapacity)
{
    static_assert(castle::semaphore<4>::max() == 4U, "max");
    static_assert(castle::semaphore<1>::max() == 1U, "max");
}


// ---------------------------------------------------------------------------
// Construction clamps an out-of-range initial count instead of invoking
// undefined behavior.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, ConstructionClampsInitialCount)
{
    castle::semaphore<4> s(100U);

    EXPECT_EQ(s.count(), 4U);
}


// ---------------------------------------------------------------------------
// try_acquire() succeeds while permits remain and fails once exhausted.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, TryAcquireRespectsCapacity)
{
    castle::semaphore<4> s(4U);

    EXPECT_EQ(s.count(), 4U);

    EXPECT_TRUE(s.try_acquire());
    EXPECT_TRUE(s.try_acquire());
    EXPECT_TRUE(s.try_acquire());
    EXPECT_TRUE(s.try_acquire());

    EXPECT_EQ(s.count(), 0U);

    // Capacity is exhausted.
    EXPECT_FALSE(s.try_acquire());
}


// ---------------------------------------------------------------------------
// release() returns permits and try_acquire() can observe them again.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, ReleaseReturnsPermits)
{
    castle::semaphore<4> s(0U);

    EXPECT_EQ(s.count(), 0U);
    EXPECT_FALSE(s.try_acquire());

    EXPECT_TRUE(s.release());
    EXPECT_EQ(s.count(), 1U);

    EXPECT_TRUE(s.try_acquire());
    EXPECT_EQ(s.count(), 0U);
}


// ---------------------------------------------------------------------------
// release() with an explicit update count.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, ReleaseWithExplicitUpdate)
{
    castle::semaphore<4> s(0U);

    EXPECT_TRUE(s.release(3U));
    EXPECT_EQ(s.count(), 3U);
}


// ---------------------------------------------------------------------------
// release() fails, and leaves the count unchanged, when it would exceed
// MaxCount rather than saturating or invoking undefined behavior.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, ReleaseRejectsOverflow)
{
    castle::semaphore<4> s(4U);

    EXPECT_FALSE(s.release());
    EXPECT_EQ(s.count(), 4U);

    EXPECT_TRUE(s.try_acquire());
    EXPECT_FALSE(s.release(2U));
    EXPECT_EQ(s.count(), 3U);

    EXPECT_TRUE(s.release(1U));
    EXPECT_EQ(s.count(), 4U);
}


// ---------------------------------------------------------------------------
// Single-permit configuration behaves like a binary flag.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, SinglePermitConfiguration)
{
    castle::semaphore<1> s(1U);

    EXPECT_TRUE(s.try_acquire());
    EXPECT_FALSE(s.try_acquire());

    EXPECT_TRUE(s.release());
    EXPECT_FALSE(s.release());

    EXPECT_TRUE(s.try_acquire());
}


// ---------------------------------------------------------------------------
// castle::binary_semaphore is an alias for semaphore<1>.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, BinarySemaphoreAlias)
{
    castle::binary_semaphore s(0U);

    EXPECT_FALSE(s.try_acquire());
    EXPECT_TRUE(s.release());
    EXPECT_TRUE(s.try_acquire());
    EXPECT_FALSE(s.try_acquire());
}


// ---------------------------------------------------------------------------
// acquire() blocking path.
//
// No permits are available. A waiting thread must not proceed until
// release() hands back a permit.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, AcquireBlocksUntilReleased)
{
    castle::semaphore<2> s(0U);

    bool waiter_started = false;
    bool waiter_finished = false;

    std::thread waiter(
        [&s, &waiter_started, &waiter_finished]()
        {
            waiter_started = true;

            s.acquire();

            waiter_finished = true;
        });

    while (!waiter_started)
    {
        std::this_thread::yield();
    }

    // Give the waiter a chance to spin; it must still be blocked.
    std::this_thread::yield();

    EXPECT_FALSE(waiter_finished);

    EXPECT_TRUE(s.release());

    waiter.join();

    EXPECT_TRUE(waiter_finished);
}


// ---------------------------------------------------------------------------
// A bounded pool of permits never admits more concurrent holders than
// MaxCount, exercised with multiple worker threads.
// ---------------------------------------------------------------------------
TEST(SemaphoreTest, BoundedConcurrentAccess)
{
    static CASTLE_CONSTEXPR uint32_t max_permits = 3U;
    static CASTLE_CONSTEXPR uint32_t worker_count = 8U;

    castle::semaphore<max_permits> s(max_permits);

    std::atomic<uint32_t> concurrent{0U};
    std::atomic<uint32_t> observed_max{0U};

    std::thread workers[worker_count];

    for (uint32_t i = 0U; i < worker_count; ++i)
    {
        workers[i] = std::thread(
            [&s, &concurrent, &observed_max]()
            {
                s.acquire();

                uint32_t now = ++concurrent;

                uint32_t prev = observed_max.load();
                while (now > prev && !observed_max.compare_exchange_weak(prev, now))
                {
                }

                --concurrent;

                EXPECT_TRUE(s.release());
            });
    }

    for (uint32_t i = 0U; i < worker_count; ++i)
    {
        workers[i].join();
    }

    EXPECT_LE(observed_max.load(), max_permits);
    EXPECT_EQ(s.count(), max_permits);
}


// ---------------------------------------------------------------------------
// A custom WaitPolicy is invoked on each failed retry of a blocked
// acquire(), instead of castle::spin_wait's default CPU-relax hint. This is
// the extension point an RTOS integration would use to yield the calling
// task to a real scheduler rather than spin.
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

TEST(SemaphoreTest, CustomWaitPolicyInvokedWhileBlocked)
{
    counting_wait_policy::hit_count.store(0U);

    castle::semaphore<1, counting_wait_policy> s(0U);

    static_assert(castle::is_same<decltype(s)::wait_policy_type, counting_wait_policy>::value,
                  "wait_policy_type");

    bool waiter_started = false;
    bool waiter_finished = false;

    std::thread waiter(
        [&s, &waiter_started, &waiter_finished]()
        {
            waiter_started = true;

            s.acquire();

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

    EXPECT_TRUE(s.release());

    waiter.join();

    EXPECT_TRUE(waiter_finished);
}

} // namespace
