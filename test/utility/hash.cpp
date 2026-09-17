#include <gtest/gtest.h>

#include "castle/utility/hash.h"
#include "castle/container/string_view.h"

#include <stdint.h>

namespace
{

enum PlainEnum
{
    EnumA = 1,
    EnumB = 2
};

enum class ScopedEnum : uint16_t
{
    X = 10,
    Y = 20
};

TEST(HashTest, IntegralDeterministic)
{
    castle::hash<int> hasher;
    EXPECT_EQ(hasher(42), hasher(42));
}

TEST(HashTest, IntegralDistinguishesValues)
{
    castle::hash<int> hasher;
    EXPECT_NE(hasher(1), hasher(2));
}

TEST(HashTest, UnsignedIntegral)
{
    castle::hash<uint64_t> hasher;
    EXPECT_EQ(hasher(0xDEADBEEFULL), hasher(0xDEADBEEFULL));
    EXPECT_NE(hasher(0ULL), hasher(1ULL));
}

TEST(HashTest, PlainEnum)
{
    castle::hash<PlainEnum> hasher;
    EXPECT_EQ(hasher(EnumA), hasher(EnumA));
    EXPECT_NE(hasher(EnumA), hasher(EnumB));
}

TEST(HashTest, ScopedEnum)
{
    castle::hash<ScopedEnum> hasher;
    EXPECT_EQ(hasher(ScopedEnum::X), hasher(ScopedEnum::X));
    EXPECT_NE(hasher(ScopedEnum::X), hasher(ScopedEnum::Y));
}

TEST(HashTest, Pointer)
{
    int a = 0;
    int b = 0;
    castle::hash<int*> hasher;
    EXPECT_EQ(hasher(&a), hasher(&a));
    EXPECT_NE(hasher(&a), hasher(&b));
}

TEST(HashTest, NullPointer)
{
    castle::hash<int*> hasher;
    int* null_ptr = nullptr;
    EXPECT_EQ(hasher(null_ptr), hasher(null_ptr));
}

TEST(HashTest, ConstQualifiedForwardsToBase)
{
    castle::hash<int> plain;
    castle::hash<const int> qualified;
    const int value = 7;
    EXPECT_EQ(qualified(value), plain(7));
}

TEST(HashTest, StringViewDeterministic)
{
    castle::hash<castle::container::string_view> hasher;
    castle::container::string_view a("hello");
    castle::container::string_view b("hello");
    EXPECT_EQ(hasher(a), hasher(b));
}

TEST(HashTest, StringViewDistinguishesContent)
{
    castle::hash<castle::container::string_view> hasher;
    castle::container::string_view a("hello");
    castle::container::string_view b("world");
    EXPECT_NE(hasher(a), hasher(b));
}

TEST(HashTest, EmptyStringView)
{
    castle::hash<castle::container::string_view> hasher;
    castle::container::string_view empty;
    EXPECT_EQ(hasher(empty), hasher(empty));
}

TEST(HashTest, OperatorIsNoexcept)
{
    castle::hash<int> hasher;
    static_assert(noexcept(hasher(0)), "hash operator must be noexcept");
}

} // namespace
