#include <gtest/gtest.h>

#include "castle/utility/type_info.hpp"

#include <string.h>

namespace
{

struct Plain
{
    int value;
};

namespace inner
{
template <typename T>
struct Box
{
    T item;
};
enum class Mode : unsigned char
{
    idle
};
} // namespace inner

TEST(TypeInfoTest, OneObjectPerTypeIgnoringCvAndReferences)
{
    EXPECT_EQ(&castle::type_id<int>(), &castle::type_id<const int>());
    EXPECT_EQ(&castle::type_id<int>(), &castle::type_id<volatile int>());
    EXPECT_EQ(&castle::type_id<int>(), &castle::type_id<const volatile int&>());
    EXPECT_EQ(&castle::type_id<int>(), &castle::type_id<int&&>());
    EXPECT_EQ(&castle::type_id<Plain>(), &castle::type_id<const Plain&>());
}

TEST(TypeInfoTest, DistinctTypesHaveDistinctIdentities)
{
    // Pairs with identical size/layout must still differ.
    EXPECT_NE(&castle::type_id<int>(), &castle::type_id<unsigned>());
    EXPECT_NE(&castle::type_id<int>(), &castle::type_id<int*>());
    EXPECT_NE(&castle::type_id<int*>(), &castle::type_id<const int*>());
    EXPECT_NE(&castle::type_id<int>(), &castle::type_id<inner::Mode>());
    EXPECT_NE(&castle::type_id<inner::Box<int>>(), &castle::type_id<inner::Box<unsigned>>());
    EXPECT_NE(&castle::type_id<void>(), &castle::type_id<Plain>());
}

TEST(TypeInfoTest, EqualityOperators)
{
    const castle::type_info& a = castle::type_id<int>();
    const castle::type_info& b = castle::type_id<long>();
    EXPECT_TRUE(a == castle::type_id<const int&>());
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a != castle::type_id<int>());
}

TEST(TypeInfoTest, BeforeIsStrictTotalOrder)
{
    const castle::type_info& a = castle::type_id<int>();
    const castle::type_info& b = castle::type_id<float>();
    const castle::type_info& c = castle::type_id<Plain>();

    EXPECT_FALSE(a.before(a));                 // irreflexive
    EXPECT_NE(a.before(b), b.before(a));       // exactly one direction for distinct types
    EXPECT_NE(b.before(c), c.before(b));
    if (a.before(b) && b.before(c))            // transitive
    {
        EXPECT_TRUE(a.before(c));
    }
    if (c.before(b) && b.before(a))
    {
        EXPECT_TRUE(c.before(a));
    }
}

TEST(TypeInfoTest, HashCodeConsistentWithEquality)
{
    EXPECT_EQ(castle::type_id<int>().hash_code(), castle::type_id<const int&>().hash_code());
    EXPECT_EQ(castle::type_id<Plain>().hash_code(), castle::type_id<Plain>().hash_code());
    EXPECT_NE(castle::type_id<int>().hash_code(), castle::type_id<unsigned>().hash_code());
    EXPECT_EQ(castle::type_id<int>().hash_code(),
              castle::hash<const castle::type_info*>()(&castle::type_id<int>()));
}

TEST(TypeInfoTest, NamesAreExtractedAndNullTerminated)
{
    EXPECT_STREQ(castle::type_id<int>().name(), "int");
    EXPECT_STREQ(castle::type_id<const int&>().name(), "int");
    EXPECT_STREQ(castle::type_id<void>().name(), "void");
    EXPECT_STREQ(castle::type_id<double>().name(), "double");

    const char* plain = castle::type_id<Plain>().name();
    EXPECT_NE(strstr(plain, "Plain"), nullptr);
    EXPECT_EQ(strstr(plain, "]"), nullptr);    // no signature residue

    const char* box = castle::type_id<inner::Box<int>>().name();
    EXPECT_NE(strstr(box, "inner::Box<int>"), nullptr);
    EXPECT_NE(strstr(castle::type_id<inner::Mode>().name(), "inner::Mode"), nullptr);
    EXPECT_NE(strstr(castle::type_id<const char*>().name(), "char"), nullptr);
}

TEST(TypeInfoTest, NameStorageIsSharedPerType)
{
    EXPECT_EQ(castle::type_id<Plain>().name(), castle::type_id<const Plain>().name());
}

TEST(TypeInfoTest, TypeIdIsConstantExpression)
{
    static constexpr const castle::type_info& info = castle::type_id<Plain>();
    EXPECT_EQ(&info, &castle::type_id<Plain>());
}

} // namespace
