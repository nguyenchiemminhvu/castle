#include <gtest/gtest.h>

#include "castle/utility/type_index.hpp"
#include "castle/utility/compare.hpp"

namespace
{

struct Payload
{
    int value;
};

TEST(TypeIndexTest, ImplicitConstructionAndCopy)
{
    castle::type_index index = castle::type_id<Payload>();
    castle::type_index copy = index;
    EXPECT_TRUE(copy == index);
    EXPECT_TRUE(index == castle::type_id<const Payload&>());

    copy = castle::type_id<int>();
    EXPECT_TRUE(copy == castle::type_id<int>());
    EXPECT_TRUE(copy != index);
    EXPECT_EQ(sizeof(castle::type_index), sizeof(void*));
}

TEST(TypeIndexTest, ConstexprConstruction)
{
    static constexpr castle::type_index index = castle::type_id<Payload>();
    EXPECT_TRUE(index == castle::type_id<Payload>());
}

TEST(TypeIndexTest, Equality)
{
    castle::type_index a = castle::type_id<int>();
    castle::type_index b = castle::type_id<unsigned>();
    EXPECT_TRUE(a == castle::type_index(castle::type_id<int>()));
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a != castle::type_index(castle::type_id<int>()));
}

TEST(TypeIndexTest, OrderingMatchesBefore)
{
    castle::type_index a = castle::type_id<int>();
    castle::type_index b = castle::type_id<float>();
    castle::type_index lo = castle::type_id<int>().before(castle::type_id<float>()) ? a : b;
    castle::type_index hi = (lo == a) ? b : a;

    EXPECT_TRUE(lo < hi);
    EXPECT_FALSE(hi < lo);
    EXPECT_FALSE(lo < lo);

    EXPECT_TRUE(lo <= hi);
    EXPECT_FALSE(hi <= lo);
    EXPECT_TRUE(lo <= lo);

    EXPECT_TRUE(hi > lo);
    EXPECT_FALSE(lo > hi);
    EXPECT_FALSE(lo > lo);

    EXPECT_TRUE(hi >= lo);
    EXPECT_FALSE(lo >= hi);
    EXPECT_TRUE(lo >= lo);

    EXPECT_TRUE(castle::less<castle::type_index>()(lo, hi));
    EXPECT_TRUE(castle::equal_to<castle::type_index>()(lo, castle::type_index(lo)));
}

TEST(TypeIndexTest, HashAndNameForwardToTypeInfo)
{
    castle::type_index index = castle::type_id<Payload>();
    EXPECT_EQ(index.hash_code(), castle::type_id<Payload>().hash_code());
    EXPECT_EQ(castle::hash<castle::type_index>()(index), index.hash_code());
    EXPECT_NE(castle::hash<castle::type_index>()(castle::type_id<int>()),
              castle::hash<castle::type_index>()(castle::type_id<unsigned>()));
    EXPECT_EQ(index.name(), castle::type_id<Payload>().name());
    EXPECT_STREQ(castle::type_index(castle::type_id<int>()).name(), "int");
}

TEST(TypeIndexTest, LookupTable)
{
    const castle::type_index keys[] = { castle::type_id<char>(), castle::type_id<short>(), castle::type_id<Payload>() };
    castle::type_index query = castle::type_id<const Payload&>();
    int found = -1;
    for (int i = 0; i < 3; ++i)
    {
        if (keys[i] == query)
        {
            found = i;
        }
    }
    EXPECT_EQ(found, 2);
}

} // namespace
