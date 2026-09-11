#include <gtest/gtest.h>

#include "castle/utility/optional.h"
#include "castle/core/traits.h"

namespace
{

struct Point
{
    int x;
    int y;
    Point(int x_value, int y_value) : x(x_value), y(y_value) {}
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator<(const Point& other) const
    {
        return (x != other.x) ? (x < other.x) : (y < other.y);
    }
};

struct Tracker
{
    int moves = 0;
    int copies = 0;
    Tracker() = default;
    Tracker(const Tracker& other) : moves(other.moves), copies(other.copies + 1) {}
    Tracker(Tracker&& other) noexcept : moves(other.moves + 1), copies(other.copies) {}
    Tracker& operator=(const Tracker&) = default;
    Tracker& operator=(Tracker&&) = default;
};

TEST(OptionalTest, DefaultConstructedIsEmpty)
{
    castle::optional<int> opt;
    EXPECT_FALSE(opt.has_value());
    EXPECT_FALSE(static_cast<bool>(opt));
}

TEST(OptionalTest, NulloptConstructedIsEmpty)
{
    castle::optional<int> opt(castle::nullopt);
    EXPECT_FALSE(opt.has_value());
}

TEST(OptionalTest, ValueConstruction)
{
    castle::optional<int> opt(42);
    EXPECT_TRUE(opt.has_value());
    EXPECT_TRUE(static_cast<bool>(opt));
    EXPECT_EQ(opt.value(), 42);
    EXPECT_EQ(*opt, 42);
}

TEST(OptionalTest, ArrowOperator)
{
    castle::optional<Point> opt(Point(1, 2));
    EXPECT_EQ(opt->x, 1);
    EXPECT_EQ(opt->y, 2);
}

TEST(OptionalTest, InPlaceConstruction)
{
    castle::optional<Point> opt(castle::meta::in_place, 3, 4);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->x, 3);
    EXPECT_EQ(opt->y, 4);
}

TEST(OptionalTest, CopyConstruction)
{
    castle::optional<int> source(7);
    castle::optional<int> copy(source);
    EXPECT_TRUE(copy.has_value());
    EXPECT_EQ(copy.value(), 7);
    EXPECT_TRUE(source.has_value());
}

TEST(OptionalTest, CopyOfEmptyStaysEmpty)
{
    castle::optional<int> source;
    castle::optional<int> copy(source);
    EXPECT_FALSE(copy.has_value());
}

TEST(OptionalTest, MoveConstructionMovesValue)
{
    castle::optional<Tracker> source(Tracker{});
    castle::optional<Tracker> moved(castle::move(source));
    ASSERT_TRUE(moved.has_value());
    EXPECT_GE(moved->moves, 1);
}

TEST(OptionalTest, CopyAssignment)
{
    castle::optional<int> a(1);
    castle::optional<int> b;
    b = a;
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(b.value(), 1);
}

TEST(OptionalTest, AssignNulloptClears)
{
    castle::optional<int> opt(5);
    opt = castle::nullopt;
    EXPECT_FALSE(opt.has_value());
}

TEST(OptionalTest, ValueAssignment)
{
    castle::optional<int> opt;
    opt = 99;
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ(opt.value(), 99);
}

TEST(OptionalTest, Reset)
{
    castle::optional<int> opt(3);
    opt.reset();
    EXPECT_FALSE(opt.has_value());
}

TEST(OptionalTest, Emplace)
{
    castle::optional<Point> opt;
    Point& ref = opt.emplace(5, 6);
    EXPECT_EQ(ref.x, 5);
    EXPECT_EQ(opt->y, 6);
    EXPECT_TRUE(opt.has_value());
}

TEST(OptionalTest, EmplaceReplacesExisting)
{
    castle::optional<int> opt(1);
    opt.emplace(2);
    EXPECT_EQ(opt.value(), 2);
}

TEST(OptionalTest, ValueOrReturnsValueWhenEngaged)
{
    castle::optional<int> opt(10);
    EXPECT_EQ(opt.value_or(99), 10);
}

TEST(OptionalTest, ValueOrReturnsDefaultWhenEmpty)
{
    castle::optional<int> opt;
    EXPECT_EQ(opt.value_or(99), 99);
}

TEST(OptionalTest, ModifyThroughDereference)
{
    castle::optional<int> opt(1);
    *opt = 55;
    EXPECT_EQ(opt.value(), 55);
}

TEST(OptionalTest, SwapBothEngaged)
{
    castle::optional<int> a(1);
    castle::optional<int> b(2);
    a.swap(b);
    EXPECT_EQ(a.value(), 2);
    EXPECT_EQ(b.value(), 1);
}

TEST(OptionalTest, SwapOneEngaged)
{
    castle::optional<int> a(1);
    castle::optional<int> b;
    a.swap(b);
    EXPECT_FALSE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(b.value(), 1);
}

TEST(OptionalTest, FreeSwap)
{
    castle::optional<int> a(3);
    castle::optional<int> b(4);
    castle::swap(a, b);
    EXPECT_EQ(a.value(), 4);
    EXPECT_EQ(b.value(), 3);
}

TEST(OptionalTest, IterateEngaged)
{
    castle::optional<int> opt(7);
    int count = 0;
    int seen = 0;
    for (int value : opt)
    {
        seen = value;
        ++count;
    }
    EXPECT_EQ(count, 1);
    EXPECT_EQ(seen, 7);
}

TEST(OptionalTest, IterateEmpty)
{
    castle::optional<int> opt;
    int count = 0;
    for (int value : opt)
    {
        (void)value;
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(OptionalTest, EqualityBetweenOptionals)
{
    castle::optional<int> a(1);
    castle::optional<int> b(1);
    castle::optional<int> c(2);
    castle::optional<int> empty;
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a == empty);
    EXPECT_TRUE(empty == castle::optional<int>());
}

TEST(OptionalTest, RelationalBetweenOptionals)
{
    castle::optional<int> a(1);
    castle::optional<int> b(2);
    castle::optional<int> empty;
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a >= a);
    EXPECT_TRUE(empty < a);
    EXPECT_FALSE(a < empty);
}

TEST(OptionalTest, ComparisonWithNullopt)
{
    castle::optional<int> engaged(1);
    castle::optional<int> empty;
    EXPECT_TRUE(empty == castle::nullopt);
    EXPECT_TRUE(castle::nullopt == empty);
    EXPECT_TRUE(engaged != castle::nullopt);
    EXPECT_FALSE(engaged == castle::nullopt);
    EXPECT_TRUE(castle::nullopt < engaged);
    EXPECT_FALSE(engaged < castle::nullopt);
}

TEST(OptionalTest, MakeOptionalDeducesType)
{
    auto opt = castle::make_optional(123);
    static_assert(castle::meta::is_same<decltype(opt), castle::optional<int>>::value,
                  "make_optional must deduce decayed type");
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt.value(), 123);
}

} // namespace
