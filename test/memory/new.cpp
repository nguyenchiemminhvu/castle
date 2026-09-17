#include <gtest/gtest.h>

#include "castle/memory/new.h"

namespace
{

struct Pair
{
    int a;
    int b;
    Pair(int x, int y) : a(x), b(y) {}
};

TEST(NewTest, PlacementNewConstructsScalar)
{
    alignas(int) unsigned char buf[sizeof(int)];
    int* p = ::new (static_cast<void*>(&buf[0])) int(1234);
    ASSERT_EQ(static_cast<void*>(p), static_cast<void*>(&buf[0]));
    EXPECT_EQ(*p, 1234);
}

TEST(NewTest, PlacementNewConstructsAggregateWithArgs)
{
    alignas(Pair) unsigned char buf[sizeof(Pair)];
    Pair* p = ::new (static_cast<void*>(&buf[0])) Pair(7, 8);
    EXPECT_EQ(p->a, 7);
    EXPECT_EQ(p->b, 8);
    p->~Pair();
}

TEST(NewTest, PlacementArrayNewUsesProvidedStorage)
{
    alignas(int) unsigned char buf[sizeof(int) * 3];
    int* p = ::new (static_cast<void*>(&buf[0])) int[3]{10, 20, 30};
    ASSERT_EQ(static_cast<void*>(p), static_cast<void*>(&buf[0]));
    EXPECT_EQ(p[0], 10);
    EXPECT_EQ(p[1], 20);
    EXPECT_EQ(p[2], 30);
}

} // namespace
