#include "castle_ext/data/update_set.hpp"

#include "castle/utility/integral_sequence.hpp"

#include <gtest/gtest.h>

#include <stdint.h>

namespace
{
struct imu
{
    float accel_z = 0.0F;
};

struct gnss
{
    double latitude = 0.0;
};

struct battery
{
    uint16_t millivolts = 0U;
};

using small_set = castle_ext::data::update_set<imu, gnss, battery>;

template <castle::size_type N>
struct tag
{
};

template <castle::size_type... Is>
castle_ext::data::update_set<tag<Is>...> make_tag_set(castle::sequence::index_sequence<Is...>);

// 40 types: spans two 32-bit storage words.
using wide_set = decltype(make_tag_set(castle::sequence::make_index_sequence_t<40U>{}));

template <typename T>
uint32_t type_code()
{
    return castle::meta::is_same<T, imu>::value ? 1U : (castle::meta::is_same<T, gnss>::value ? 2U : 3U);
}
} // namespace

TEST(UpdateSetTest, DefaultSetIsEmpty)
{
    const small_set set;
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0U);
    EXPECT_FALSE(set.contains<imu>());
    EXPECT_FALSE(set.contains<gnss>());
    EXPECT_FALSE(set.contains<battery>());
    EXPECT_EQ(small_set::capacity(), 3U);
    EXPECT_EQ(wide_set::capacity(), 40U);
}

TEST(UpdateSetTest, OfAndAllContainExactlyTheNamedTypes)
{
    const small_set one = small_set::of<gnss>();
    EXPECT_TRUE(one.contains<gnss>());
    EXPECT_FALSE(one.contains<imu>());
    EXPECT_FALSE(one.contains<battery>());
    EXPECT_EQ(one.size(), 1U);

    const small_set two = small_set::of<imu, battery>();
    EXPECT_TRUE(two.contains<imu>());
    EXPECT_TRUE(two.contains<battery>());
    EXPECT_FALSE(two.contains<gnss>());
    EXPECT_EQ(two.size(), 2U);

    const small_set all = small_set::all();
    EXPECT_EQ(all.size(), 3U);
    EXPECT_TRUE(all.contains<imu>() && all.contains<gnss>() && all.contains<battery>());

    EXPECT_TRUE(small_set::of<>().empty());
}

TEST(UpdateSetTest, InsertEraseAndClearChangeMembership)
{
    small_set set;
    set.insert<imu>().insert<gnss, battery>();
    EXPECT_EQ(set, small_set::all());

    set.insert<imu>();
    EXPECT_EQ(set.size(), 3U);

    set.erase<gnss>();
    EXPECT_EQ(set, (small_set::of<imu, battery>()));

    set.erase<gnss>();
    EXPECT_EQ(set.size(), 2U);

    set.erase<imu, battery>();
    EXPECT_TRUE(set.empty());

    set.insert<battery>();
    set.clear();
    EXPECT_TRUE(set.empty());
}

TEST(UpdateSetTest, IgnoresCvQualifiersOfQueriedType)
{
    small_set set;
    set.insert<const imu>();
    EXPECT_TRUE(set.contains<imu>());
    EXPECT_TRUE(set.contains<const volatile imu>());
    set.erase<volatile imu>();
    EXPECT_TRUE(set.empty());
}

TEST(UpdateSetTest, UnionIntersectionAndEquality)
{
    const small_set a = small_set::of<imu, gnss>();
    const small_set b = small_set::of<gnss, battery>();

    EXPECT_EQ(a | b, small_set::all());
    EXPECT_EQ(a & b, small_set::of<gnss>());
    EXPECT_TRUE((a & small_set::of<battery>()).empty());

    small_set c = a;
    c |= b;
    EXPECT_EQ(c, small_set::all());
    c &= small_set::of<battery>();
    EXPECT_EQ(c, small_set::of<battery>());

    EXPECT_TRUE((a == small_set::of<gnss, imu>()));
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE((a != small_set::of<imu, gnss>()));
}

TEST(UpdateSetTest, ForEachVisitsMembersInDeclarationOrder)
{
    uint32_t order[3] = {};
    uint32_t count = 0U;
    small_set::of<battery, imu>().for_each([&](auto type)
    {
        order[count] = type_code<typename decltype(type)::type>();
        ++count;
    });
    ASSERT_EQ(count, 2U);
    EXPECT_EQ(order[0], 1U);
    EXPECT_EQ(order[1], 3U);

    count = 0U;
    small_set{}.for_each([&count](auto)
    {
        ++count;
    });
    EXPECT_EQ(count, 0U);
}

TEST(UpdateSetTest, WideSetHandlesTypesInEveryStorageWord)
{
    wide_set set = wide_set::of<tag<0U>, tag<31U>, tag<32U>, tag<39U>>();
    EXPECT_EQ(set.size(), 4U);
    EXPECT_TRUE(set.contains<tag<31U>>());
    EXPECT_TRUE(set.contains<tag<32U>>());
    EXPECT_FALSE(set.contains<tag<33U>>());

    set.erase<tag<0U>, tag<31U>>();
    EXPECT_EQ(set, (wide_set::of<tag<32U>, tag<39U>>()));
    EXPECT_EQ(wide_set::all().size(), 40U);
    EXPECT_EQ(set & wide_set::of<tag<39U>>(), wide_set::of<tag<39U>>());

    uint32_t visited = 0U;
    wide_set::all().for_each([&visited](auto)
    {
        ++visited;
    });
    EXPECT_EQ(visited, 40U);
}
