#include <gtest/gtest.h>
#include "castle/sync/scoped_mutex.hpp"

namespace
{
TEST(ScopedMutexTest, RAIIReleasesLock)
{
    castle::mutex m;
    EXPECT_TRUE(m.try_lock());
    m.unlock();
    {
        castle::scoped_mutex g(m);
        EXPECT_FALSE(m.try_lock());
    }
    EXPECT_TRUE(m.try_lock());
    m.unlock();
}


// ---------------------------------------------------------------------------
// castle::scoped_mutex's WaitPolicy is threaded through from the guarded
// castle::basic_mutex; a custom policy is accepted the same way.
// ---------------------------------------------------------------------------
TEST(ScopedMutexTest, CustomWaitPolicyMutexIsUsable)
{
    struct noop_wait_policy
    {
        static void wait() noexcept {}
    };

    castle::basic_mutex<noop_wait_policy> m;
    EXPECT_TRUE(m.try_lock());
    m.unlock();

    {
        castle::basic_scoped_mutex<noop_wait_policy> g(m);
        EXPECT_FALSE(m.try_lock());
    }

    EXPECT_TRUE(m.try_lock());
    m.unlock();
}
}
