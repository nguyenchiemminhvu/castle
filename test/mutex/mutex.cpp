#include <gtest/gtest.h>
#include "castle/mutex/mutex.h"

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
}
