#include <gtest/gtest.h>
#include "castle/mutex/scoped_mutex.h"

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
}
