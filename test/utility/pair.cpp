#include <gtest/gtest.h>

#include "castle/utility/pair.h"
#include "castle/core/traits.h"

namespace
{

TEST(PairTest, ValueConstruction)
{
    castle::pair<int, char> p(1, 'a');
    EXPECT_EQ(p.first, 1);
    EXPECT_EQ(p.second, 'a');
}

TEST(PairTest, MemberTypes)
{
    using pair_type = castle::pair<int, double>;
    static_assert(castle::meta::is_same<pair_type::first_type, int>::value,
                  "first_type must match");
    static_assert(castle::meta::is_same<pair_type::second_type, double>::value,
                  "second_type must match");
}

TEST(PairTest, DefaultConstructionThenAssign)
{
    castle::pair<int, int> p;
    p.first = 8;
    p.second = 9;
    EXPECT_EQ(p.first, 8);
    EXPECT_EQ(p.second, 9);
}

TEST(PairTest, CopyConstruction)
{
    castle::pair<int, int> a(3, 4);
    castle::pair<int, int> b(a);
    EXPECT_EQ(b.first, 3);
    EXPECT_EQ(b.second, 4);
}

TEST(PairTest, CopyAssignment)
{
    castle::pair<int, int> a(5, 6);
    castle::pair<int, int> b;
    b = a;
    EXPECT_EQ(b.first, 5);
    EXPECT_EQ(b.second, 6);
}

TEST(PairTest, MakePairDeducesDecayedTypes)
{
    auto p = castle::make_pair(1, 2.5);
    static_assert(castle::meta::is_same<decltype(p), castle::pair<int, double>>::value,
                  "make_pair must decay argument types");
    EXPECT_EQ(p.first, 1);
    EXPECT_DOUBLE_EQ(p.second, 2.5);
}

TEST(PairTest, GetByIndex)
{
    castle::pair<int, char> p(10, 'z');
    EXPECT_EQ(castle::get<0>(p), 10);
    EXPECT_EQ(castle::get<1>(p), 'z');
}

TEST(PairTest, GetByIndexModifies)
{
    castle::pair<int, int> p(1, 2);
    castle::get<0>(p) = 100;
    castle::get<1>(p) = 200;
    EXPECT_EQ(p.first, 100);
    EXPECT_EQ(p.second, 200);
}

TEST(PairTest, GetOnConstPair)
{
    const castle::pair<int, char> p(7, 'q');
    EXPECT_EQ(castle::get<0>(p), 7);
    EXPECT_EQ(castle::get<1>(p), 'q');
}

TEST(PairTest, MemberSwap)
{
    castle::pair<int, int> a(1, 2);
    castle::pair<int, int> b(3, 4);
    a.swap(b);
    EXPECT_EQ(a.first, 3);
    EXPECT_EQ(a.second, 4);
    EXPECT_EQ(b.first, 1);
    EXPECT_EQ(b.second, 2);
}

TEST(PairTest, FreeSwap)
{
    castle::pair<int, int> a(1, 2);
    castle::pair<int, int> b(3, 4);
    castle::swap(a, b);
    EXPECT_EQ(a.first, 3);
    EXPECT_EQ(b.first, 1);
}

TEST(PairTest, Equality)
{
    castle::pair<int, int> a(1, 2);
    castle::pair<int, int> b(1, 2);
    castle::pair<int, int> c(1, 3);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(PairTest, Relational)
{
    castle::pair<int, int> a(1, 2);
    castle::pair<int, int> b(1, 3);
    castle::pair<int, int> c(2, 0);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a < c);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a >= a);
    EXPECT_FALSE(b < a);
}

TEST(PairTest, HeterogeneousComparison)
{
    castle::pair<int, long> a(1, 2L);
    castle::pair<long, int> b(1L, 2);
    EXPECT_TRUE(a == b);
}

} // namespace
