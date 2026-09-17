#include <gtest/gtest.h>
#include "castle/container/vector.hpp"
#include <type_traits>
namespace
{
struct Tracker
{
    static int live;
    int v;
    Tracker(int x=0)
        :v(x)
    {
        ++live;
    }
    Tracker(const Tracker&o)
        :v(o.v)
    {
        ++live;
    }
    Tracker(Tracker&&o) noexcept
        :v(o.v)
    {
        o.v=-1;
        ++live;
    }
    Tracker& operator=(const Tracker&o)
    {
        v=o.v;
        return *this;
    }
    Tracker& operator=(Tracker&&o) noexcept
    {
        v=o.v;
        o.v=-1;
        return *this;
    }
    ~Tracker(){--live;}
};

int Tracker::live=0;

TEST(VectorTest, CapacityIteratorsAndFixedOperations)
{
    using V=castle::container::vector<int,3>;
    static_assert(V::static_capacity==3U,"capacity");
    static_assert(noexcept(V{}),"ctor");
    V v;
    EXPECT_EQ(v.size(),0U);
    EXPECT_EQ(v.capacity(),3U);
    EXPECT_TRUE(v.empty());
    EXPECT_FALSE(v.full());
    EXPECT_EQ(v.begin(),v.end());
    EXPECT_EQ(v.data(),v.begin());
    EXPECT_EQ(v.push_back(1),castle::status::ok);
    int x=2;
    EXPECT_EQ(v.push_back(x),castle::status::ok);
    EXPECT_EQ(v.emplace_back(3),castle::status::ok);
    EXPECT_TRUE(v.full());
    EXPECT_EQ(v.push_back(4),castle::status::full);
    EXPECT_EQ(v.front(),1);
    EXPECT_EQ(v.back(),3);
    v[1]=7;
    EXPECT_EQ(v[1],7);
    int sum=0;
    for(auto it=v.begin();it!=v.end();++it)
    {
        sum+=*it;
    }
    EXPECT_EQ(sum,11);
    const V& cv=v;
    EXPECT_EQ(cv.cbegin(),cv.begin());
    EXPECT_EQ(cv.cend(),cv.end());
    EXPECT_EQ(*cv.rbegin(),3);
    EXPECT_EQ(cv.rbegin().base(),cv.end());
    EXPECT_EQ(cv.rend().base(),cv.begin());
    EXPECT_EQ(v.pop_back(),castle::status::ok);
    EXPECT_EQ(v.pop_back(),castle::status::ok);
    EXPECT_EQ(v.pop_back(),castle::status::ok);
    EXPECT_EQ(v.pop_back(),castle::status::empty);
    EXPECT_EQ(v.data(),v.begin());
}

TEST(VectorTest, CopyMoveAndDestructionPaths)
{
    Tracker::live=0;
    {
        castle::container::vector<Tracker,3> a;
        EXPECT_EQ(a.emplace_back(1),castle::status::ok);
        EXPECT_EQ(a.emplace_back(2),castle::status::ok);
        castle::container::vector<Tracker,3> b(a);
        EXPECT_EQ(b.size(),2U);
        EXPECT_EQ(Tracker::live,4);
        castle::container::vector<Tracker,3> c;
        c.emplace_back(9);
        c=c;
        EXPECT_EQ(c.size(),1U);
        c=b;
        EXPECT_EQ(c.size(),2U);
        EXPECT_EQ(Tracker::live,6);
        castle::container::vector<Tracker,3> d(std::move(a));
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(d.size(),2U);
        c=std::move(d);
        EXPECT_TRUE(d.empty());
        EXPECT_EQ(c.size(),2U);
    }
    EXPECT_EQ(Tracker::live,0);
}

TEST(VectorTest, InitializerListConstructorTruncatesWhenListExceedsCapacity)
{
    castle::container::vector<int, 3> v = {1, 2, 3, 4, 5};

    EXPECT_EQ(v.size(), 3U);
    EXPECT_TRUE(v.full());

    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(VectorTest, InitializerListAssignmentTruncatesWhenListExceedsCapacity)
{
    castle::container::vector<int, 3> v;

    EXPECT_EQ(v.push_back(100), castle::status::ok);
    EXPECT_EQ(v.push_back(200), castle::status::ok);

    v = {10, 20, 30, 40, 50};

    EXPECT_EQ(v.size(), 3U);
    EXPECT_TRUE(v.full());

    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
    EXPECT_EQ(v[2], 30);
}

TEST(VectorTest, InitializerListAssignmentClearsExistingElementsBeforeTruncation)
{
    Tracker::live = 0;

    {
        castle::container::vector<Tracker, 2> v;

        EXPECT_EQ(v.emplace_back(1), castle::status::ok);
        EXPECT_EQ(v.emplace_back(2), castle::status::ok);
        EXPECT_EQ(Tracker::live, 2);

        v = {Tracker(10), Tracker(20), Tracker(30)};

        EXPECT_EQ(v.size(), 2U);
        EXPECT_EQ(v[0].v, 10);
        EXPECT_EQ(v[1].v, 20);
        EXPECT_EQ(Tracker::live, 2);
    }

    EXPECT_EQ(Tracker::live, 0);
}

} // namespace
