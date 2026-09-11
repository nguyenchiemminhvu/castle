#include <gtest/gtest.h>
#include "castle/chrono/clocks.h"

TEST(ClocksTest, PropertiesNowAndTimeTConversion)
{
    static_assert(!castle::chrono::system_clock::is_steady,"system");
    static_assert(castle::chrono::steady_clock::is_steady,"steady");
    static_assert(castle::meta::is_same<castle::chrono::high_resolution_clock,castle::chrono::system_clock>::value,"alias");
    auto sn=castle::chrono::system_clock::now();
    auto st=castle::chrono::steady_clock::now();
    EXPECT_GT(sn.time_since_epoch().count(),0);
    EXPECT_GT(st.time_since_epoch().count(),0);
    ::time_t t=castle::chrono::system_clock::to_time_t(sn);
    auto back=castle::chrono::system_clock::from_time_t(t);
    EXPECT_EQ(castle::chrono::system_clock::to_time_t(back),t);
}
