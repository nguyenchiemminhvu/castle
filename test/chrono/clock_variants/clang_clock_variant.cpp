#include <gtest/gtest.h>
#include "castle/chrono/clock_variants/clang_clock_variant.h"

TEST(ClangClockVariantTest, HooksAreCallable)
{
    auto r=castle::chrono::detail::clock_variant::realtime_ns();
    auto m=castle::chrono::detail::clock_variant::monotonic_ns();
    EXPECT_GE(r.tv_sec,0);
    EXPECT_GE(r.tv_nsec,0);
    EXPECT_GE(m.tv_sec,0);
    EXPECT_GE(m.tv_nsec,0);
}
