#include <gtest/gtest.h>

#include "castle/memory/addressof.h"

namespace
{

// A type that hijacks operator& so that the built-in & would return a bogus
// pointer. addressof must ignore the overload and return the true address.
struct EvilAddress
{
    int value;

    EvilAddress* operator&() CASTLE_NOEXCEPT { return nullptr; }
    CASTLE_CONST EvilAddress* operator&() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }
};

TEST(AddressofTest, ReturnsRealAddressOfScalar)
{
    int x = 42;
    EXPECT_EQ(castle::memory::addressof(x), &x);
}

TEST(AddressofTest, ConstOverloadReturnsRealAddress)
{
    CASTLE_CONST double d = 3.14;
    EXPECT_EQ(castle::memory::addressof(d), &d);
}

TEST(AddressofTest, IgnoresOverloadedOperatorAmpersand)
{
    EvilAddress e{7};

    // The overloaded operator& yields nullptr; addressof must not.
    EXPECT_EQ(&e, nullptr);
    EvilAddress* real = castle::memory::addressof(e);
    ASSERT_NE(real, nullptr);
    EXPECT_EQ(real->value, 7);
}

TEST(AddressofTest, IgnoresOverloadedOperatorAmpersandConst)
{
    CASTLE_CONST EvilAddress e{11};

    EXPECT_EQ(&e, nullptr);
    CASTLE_CONST EvilAddress* real = castle::memory::addressof(e);
    ASSERT_NE(real, nullptr);
    EXPECT_EQ(real->value, 11);
}

TEST(AddressofTest, WorksForArrayElements)
{
    int arr[4] = {1, 2, 3, 4};
    EXPECT_EQ(castle::memory::addressof(arr[2]), &arr[2]);
    EXPECT_EQ(castle::memory::addressof(arr), &arr);
}

TEST(AddressofTest, DistinctObjectsHaveDistinctAddresses)
{
    int a = 0;
    int b = 0;
    EXPECT_NE(castle::memory::addressof(a), castle::memory::addressof(b));
}

} // namespace
