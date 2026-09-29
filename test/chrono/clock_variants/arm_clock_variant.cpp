#include <gtest/gtest.h>
#include "castle/chrono/clock_variants/arm_clock_variant.hpp"

TEST(ArmClockVariantTest, HooksAreCallable)
{
    auto r=castle::chrono::system_clock_adapter::realtime_ns();
    auto m=castle::chrono::system_clock_adapter::monotonic_ns();
    EXPECT_GE(r.tv_sec,0);
    EXPECT_GE(r.tv_nsec,0);
    EXPECT_GE(m.tv_sec,0);
    EXPECT_GE(m.tv_nsec,0);
}
