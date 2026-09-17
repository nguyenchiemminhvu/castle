#include <gtest/gtest.h>

#include "castle/utility/move.h"
#include "castle/core/traits.h"

namespace
{

struct MoveTracker
{
    int moved = 0;
    int copied = 0;

    MoveTracker() = default;
    MoveTracker(const MoveTracker& other) : moved(other.moved), copied(other.copied + 1) {}
    MoveTracker(MoveTracker&& other) noexcept : moved(other.moved + 1), copied(other.copied) {}

    MoveTracker& operator=(const MoveTracker& other)
    {
        moved = other.moved;
        copied = other.copied + 1;
        return *this;
    }
    MoveTracker& operator=(MoveTracker&& other) noexcept
    {
        moved = other.moved + 1;
        copied = other.copied;
        return *this;
    }
};

TEST(MoveTest, ProducesRvalueReference)
{
    int value = 5;
    static_assert(castle::meta::is_same<decltype(castle::move(value)), int&&>::value,
                  "castle::move(lvalue) must yield T&&");
}

TEST(MoveTest, RemovesReferenceFromLvalueReference)
{
    int value = 7;
    int& ref = value;
    static_assert(castle::meta::is_same<decltype(castle::move(ref)), int&&>::value,
                  "castle::move must strip lvalue reference");
    (void)ref;
}

TEST(MoveTest, ActuallyMovesTrackedType)
{
    MoveTracker source;
    MoveTracker destination(castle::move(source));
    EXPECT_EQ(destination.moved, 1);
    EXPECT_EQ(destination.copied, 0);
}

TEST(MoveTest, MoveAssignmentTakesMovePath)
{
    MoveTracker source;
    MoveTracker destination;
    destination = castle::move(source);
    EXPECT_EQ(destination.moved, 1);
    EXPECT_EQ(destination.copied, 0);
}

TEST(MoveTest, CopyPathWhenNotMoved)
{
    MoveTracker source;
    MoveTracker destination(source);
    EXPECT_EQ(destination.copied, 1);
    EXPECT_EQ(destination.moved, 0);
}

TEST(MoveTest, IsNoexcept)
{
    int value = 0;
    static_assert(noexcept(castle::move(value)), "castle::move must be noexcept");
    (void)value;
}

TEST(MoveTest, IsConstexpr)
{
    constexpr int value = 42;
    constexpr int moved = castle::move(value);
    static_assert(moved == 42, "castle::move must be usable in constexpr");
    EXPECT_EQ(moved, 42);
}

} // namespace
