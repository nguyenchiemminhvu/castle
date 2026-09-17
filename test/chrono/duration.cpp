#include <gtest/gtest.h>
#include "castle/chrono/duration.h"

namespace
{
using namespace castle::chrono;

TEST(DurationTest, ConstructionArithmeticAndValues)
{
    constexpr milliseconds z{};
    static_assert(z.count()==0,"zero");
    static_assert(milliseconds(5).count()==5,"count");
    static_assert(milliseconds::zero().count()==0,"zero fn");
    EXPECT_EQ(milliseconds::min().count(),castle::numeric_limits<int64_t>::lowest());
    EXPECT_EQ(milliseconds::max().count(),castle::numeric_limits<int64_t>::max());
    milliseconds d(10);
    EXPECT_EQ(d.count(),10);
    EXPECT_EQ((++d).count(),11);
    EXPECT_EQ((d++).count(),11);
    EXPECT_EQ(d.count(),12);
    EXPECT_EQ((--d).count(),11);
    EXPECT_EQ((d--).count(),11);
    EXPECT_EQ(d.count(),10);
    milliseconds e(3);
    EXPECT_EQ((d+=e).count(),13);
    EXPECT_EQ((d-=e).count(),10);
    EXPECT_EQ((d*=2).count(),20);
    EXPECT_EQ((d/=4).count(),5);
    EXPECT_EQ((d%=2).count(),1);
    EXPECT_EQ((d%=milliseconds(1)).count(),0);
}

TEST(DurationTest, CastCommonArithmeticComparisonAndRounding)
{
    constexpr milliseconds ms(1500);
    constexpr seconds s=duration_cast<seconds>(ms);
    static_assert(s.count()==1,"cast");
    EXPECT_EQ(duration_cast<microseconds>(ms).count(),1500000);
    EXPECT_EQ((milliseconds(500)+seconds(1)).count(),1500);
    EXPECT_EQ((seconds(2)-milliseconds(500)).count(),1500);
    EXPECT_EQ((milliseconds(4)*3).count(),12);
    EXPECT_EQ((3*milliseconds(4)).count(),12);
    EXPECT_EQ((milliseconds(9)/2).count(),4);
    EXPECT_EQ((milliseconds(10)%3).count(),1);
    EXPECT_TRUE(milliseconds(1)==microseconds(1000));
    EXPECT_TRUE(milliseconds(1)!=milliseconds(2));
    EXPECT_TRUE(milliseconds(1)<seconds(1));
    EXPECT_TRUE(milliseconds(1)<=seconds(1));
    EXPECT_TRUE(seconds(1)>milliseconds(1));
    EXPECT_TRUE(seconds(1)>=milliseconds(1));
    EXPECT_EQ(castle::chrono::floor<seconds>(milliseconds(1500)).count(),1);
    EXPECT_EQ(castle::chrono::floor<seconds>(milliseconds(2000)).count(),2);
    EXPECT_EQ(castle::chrono::ceil<seconds>(milliseconds(1500)).count(),2);
    EXPECT_EQ(castle::chrono::ceil<seconds>(milliseconds(2000)).count(),2);
    EXPECT_EQ(castle::chrono::round<seconds>(milliseconds(1400)).count(),1);
    EXPECT_EQ(castle::chrono::round<seconds>(milliseconds(1600)).count(),2);
    EXPECT_EQ(castle::chrono::round<seconds>(milliseconds(2500)).count(),2);
    EXPECT_EQ(castle::chrono::round<seconds>(milliseconds(3500)).count(),4);
    EXPECT_EQ(castle::chrono::abs(milliseconds(-7)).count(),7);
    EXPECT_EQ(castle::chrono::abs(milliseconds(7)).count(),7);
    EXPECT_EQ(+milliseconds(4).count(),4);
    EXPECT_EQ((-milliseconds(4)).count(),-4);
    static_assert(castle::is_same<castle::chrono::seconds::period,castle::seconds>::value,"period");
}
}
