#include <gtest/gtest.h>

#include "castle/utility/swap.h"

namespace
{

TEST(SwapTest, SwapsIntegers)
{
    int a = 1;
    int b = 2;
    castle::swap(a, b);
    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 1);
}

struct MoveOnly
{
    int value;
    int moves = 0;

    explicit MoveOnly(int v) : value(v) {}
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&& other) noexcept : value(other.value), moves(other.moves + 1) {}
    MoveOnly& operator=(MoveOnly&& other) noexcept
    {
        value = other.value;
        moves = other.moves + 1;
        return *this;
    }
};

TEST(SwapTest, SwapsMoveOnlyType)
{
    MoveOnly a(10);
    MoveOnly b(20);
    castle::swap(a, b);
    EXPECT_EQ(a.value, 20);
    EXPECT_EQ(b.value, 10);
}

TEST(SwapTest, SelfSwapKeepsValue)
{
    int a = 5;
    castle::swap(a, a);
    EXPECT_EQ(a, 5);
}

TEST(SwapTest, NoexceptForNothrowMovableType)
{
    int a = 0;
    int b = 0;
    static_assert(noexcept(castle::swap(a, b)),
                  "swap of trivially movable type must be noexcept");
    (void)a;
    (void)b;
}

} // namespace
