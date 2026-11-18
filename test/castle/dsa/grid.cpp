#include <gtest/gtest.h>

#include "castle/dsa/grid.hpp"

namespace
{
struct cell
{
    int id;
    int weight;

    bool operator==(const cell& other) const
    {
        return id == other.id && weight == other.weight;
    }
};

TEST(GridTest, ConstexprConstructionAndRowMajorLayout)
{
    constexpr castle::dsa::grid<int, 2U, 3U> value(1, 2, 3, 4, 5, 6);
    static_assert(value.rows() == 2U && value.columns() == 3U && value.size() == 6U, "dimensions");
    static_assert(!value.empty(), "grid is never empty");
    static_assert(value(1U, 0U) == 4, "row-major element access");
    static_assert(castle::dsa::grid<int, 2U, 3U>::static_size == 6U, "static size");
    static_assert(sizeof(castle::dsa::grid<uint16_t, 3U, 4U>) == sizeof(uint16_t) * 12U,
                  "grid adds no storage overhead");

    EXPECT_EQ(value.at(0U, 2U), 3);
    EXPECT_EQ(value.data()[1U * 3U + 2U], 6);
    EXPECT_EQ(&value(1U, 1U), value.data() + 4);
}

TEST(GridTest, DefaultConstructionValueInitializes)
{
    const castle::dsa::grid<double, 2U, 2U> zeros;
    for (double element : zeros)
    {
        EXPECT_EQ(element, 0.0);
    }
}

TEST(GridTest, InitializerListAndNonArithmeticElements)
{
    castle::dsa::grid<cell, 1U, 2U> cells{cell{1, 10}, cell{2, 20}};
    EXPECT_EQ(cells(0U, 1U).id, 2);
    cells.at(0U, 0U).weight = 15;
    EXPECT_EQ(cells(0U, 0U).weight, 15);
}

TEST(GridTest, IteratorsVisitRowMajorOrder)
{
    castle::dsa::grid<int, 2U, 2U> value(1, 2, 3, 4);
    int forward = 0;
    for (int element : value)
    {
        forward = forward * 10 + element;
    }
    EXPECT_EQ(forward, 1234);

    int backward = 0;
    for (auto it = value.crbegin(); it != value.crend(); ++it)
    {
        backward = backward * 10 + *it;
    }
    EXPECT_EQ(backward, 4321);
    EXPECT_EQ(value.cend() - value.cbegin(), 4);
    EXPECT_EQ(*value.rbegin(), 4);
    using grid_type = castle::dsa::grid<int, 2U, 2U>;
    EXPECT_EQ(value.rend(), grid_type::reverse_iterator(value.begin()));
}

TEST(GridTest, RowViewsAliasStorage)
{
    castle::dsa::grid<int, 3U, 2U> value(1, 2, 3, 4, 5, 6);
    castle::dsa::array_view<int> middle = value.row(1U);
    ASSERT_EQ(middle.size(), 2U);
    EXPECT_EQ(middle[0U], 3);
    middle[1U] = 40;
    EXPECT_EQ(value(1U, 1U), 40);

    const castle::dsa::grid<int, 3U, 2U>& view = value;
    castle::dsa::array_view<const int> last = view.row(2U);
    EXPECT_EQ(last.data(), value.data() + 4);
    EXPECT_EQ(last.back(), 6);
}

TEST(GridTest, FillAndEquality)
{
    castle::dsa::grid<uint8_t, 2U, 2U> first;
    first.fill(7U);
    const castle::dsa::grid<uint8_t, 2U, 2U> sevens(7U, 7U, 7U, 7U);
    EXPECT_TRUE(first == sevens);
    EXPECT_FALSE(first != sevens);

    first(1U, 1U) = 8U;
    EXPECT_FALSE(first == sevens);
    EXPECT_TRUE(first != sevens);
}
}
