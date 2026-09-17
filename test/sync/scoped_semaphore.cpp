#include <gtest/gtest.h>

#include "castle/sync/scoped_semaphore.hpp"

namespace
{

TEST(ScopedSemaphoreTest, RAIIAcquiresAndReleasesPermit)
{
    castle::semaphore<1> s(1U);

    EXPECT_EQ(s.count(), 1U);

    {
        castle::scoped_semaphore<1> guard(s);

        EXPECT_EQ(s.count(), 0U);
        EXPECT_FALSE(s.try_acquire());
    }

    EXPECT_EQ(s.count(), 1U);
    EXPECT_TRUE(s.try_acquire());
    EXPECT_TRUE(s.release());
}


TEST(ScopedSemaphoreTest, MultiplePermitsAllowNestedGuards)
{
    castle::semaphore<2> s(2U);

    castle::scoped_semaphore<2> outer(s);
    EXPECT_EQ(s.count(), 1U);

    {
        castle::scoped_semaphore<2> inner(s);
        EXPECT_EQ(s.count(), 0U);
    }

    EXPECT_EQ(s.count(), 1U);
}

} // namespace
