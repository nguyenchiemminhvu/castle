#include <gtest/gtest.h>
#include "castle/chrono/literals.h"

namespace
{
using namespace castle::chrono::literals::chrono_literals;

TEST(DurationLiteralsTest, AllLiteralUnits)
{
    constexpr auto ns=1_ns;
    constexpr auto us=2_us;
    constexpr auto ms=3_ms;
    constexpr auto sec=4_s;
    constexpr auto minv=5_min;
    constexpr auto h=6_h;
    constexpr auto d=7_d;
    constexpr auto w=8_w;
    static_assert(ns.count()==1,"ns");
    static_assert(us.count()==2,"us");
    static_assert(ms.count()==3,"ms");
    static_assert(sec.count()==4,"s");
    static_assert(minv.count()==5,"min");
    static_assert(h.count()==6,"h");
    static_assert(d.count()==7,"d");
    static_assert(w.count()==8,"w");
    EXPECT_EQ(ns.count(),1);
    EXPECT_EQ(us.count(),2);
    EXPECT_EQ(ms.count(),3);
    EXPECT_EQ(sec.count(),4);
    EXPECT_EQ(minv.count(),5);
    EXPECT_EQ(h.count(),6);
    EXPECT_EQ(d.count(),7);
    EXPECT_EQ(w.count(),8);
}
}
