#include <gtest/gtest.h>
#include "castle/chrono/time_point.h"

namespace 
{
struct C
{
    using duration=castle::chrono::milliseconds;
};

using TP=castle::chrono::time_point<C,castle::chrono::milliseconds>;

TEST(TimePointTest, ConstructionMutationAndCast)
{
    constexpr TP zero{};
    static_assert(zero.time_since_epoch().count()==0,"zero");
    EXPECT_EQ(TP::min().time_since_epoch().count(),castle::chrono::milliseconds::min().count());
    EXPECT_EQ(TP::max().time_since_epoch().count(),castle::chrono::milliseconds::max().count());
    TP p(castle::chrono::milliseconds(10));
    EXPECT_EQ(p.time_since_epoch().count(),10);
    EXPECT_EQ((p+=castle::chrono::milliseconds(5)).time_since_epoch().count(),15);
    EXPECT_EQ((p-=castle::chrono::milliseconds(2)).time_since_epoch().count(),13);
    EXPECT_EQ((++p).time_since_epoch().count(),14);
    EXPECT_EQ((p++).time_since_epoch().count(),14);
    EXPECT_EQ(p.time_since_epoch().count(),15);
    EXPECT_EQ((--p).time_since_epoch().count(),14);
    EXPECT_EQ((p--).time_since_epoch().count(),14);
    TP q(castle::chrono::milliseconds(2000));
    EXPECT_EQ(q.time_since_epoch().count(),2000);
    auto cast=castle::chrono::time_point_cast<castle::chrono::seconds>(q);
    EXPECT_EQ(cast.time_since_epoch().count(),2);
    auto fl=castle::chrono::floor<castle::chrono::seconds>(TP(castle::chrono::milliseconds(2500)));
    EXPECT_EQ(fl.time_since_epoch().count(),2);
    auto ce=castle::chrono::ceil<castle::chrono::seconds>(TP(castle::chrono::milliseconds(2500)));
    EXPECT_EQ(ce.time_since_epoch().count(),3);
    auto ro=castle::chrono::round<castle::chrono::seconds>(TP(castle::chrono::milliseconds(2500)));
    EXPECT_EQ(ro.time_since_epoch().count(),2);
}

TEST(TimePointTest, ArithmeticDifferenceAndComparison)
{
    TP a(castle::chrono::milliseconds(10)), b(castle::chrono::milliseconds(20));
    auto c=a+castle::chrono::seconds(1);
    EXPECT_EQ(c.time_since_epoch().count(),1010);
    auto d=castle::chrono::seconds(1)+a;
    EXPECT_EQ(d.time_since_epoch().count(),1010);
    auto e=b-castle::chrono::seconds(1);
    EXPECT_EQ(e.time_since_epoch().count(),-980);
    EXPECT_EQ((b-a).count(),10);
    EXPECT_EQ((a-b).count(),-10);
    EXPECT_FALSE(a==b);
    EXPECT_TRUE(a!=b);
    EXPECT_TRUE(a<b);
    EXPECT_TRUE(a<=b);
    EXPECT_TRUE(b>a);
    EXPECT_TRUE(b>=a);
    EXPECT_TRUE(a<=a);
    EXPECT_TRUE(a>=a);
    EXPECT_TRUE(a==TP(castle::chrono::milliseconds(10)));
}
}
