#include <gtest/gtest.h>

#include "castle/utility/macros.h"

#include <string.h>
#include <wchar.h>

namespace
{

TEST(MacrosTest, StringifyExpandsArguments)
{
    EXPECT_STREQ(CASTLE_STRINGIFY(hello), "hello");
    EXPECT_STREQ(CASTLE_STRINGIFY(1 + 2), "1 + 2");
}

#define CASTLE_TEST_INNER 99
TEST(MacrosTest, StringifyExpandsMacroValues)
{
    EXPECT_STREQ(CASTLE_STRINGIFY(CASTLE_TEST_INNER), "99");
}

TEST(MacrosTest, ConcatJoinsTokens)
{
    const int abc = 7;
    EXPECT_EQ(CASTLE_CONCAT(a, bc), abc);
}

TEST(MacrosTest, ConcatExpandsMacroValues)
{
    const int value99 = 123;
    EXPECT_EQ(CASTLE_CONCAT(value, CASTLE_TEST_INNER), value99);
}
#undef CASTLE_TEST_INNER

TEST(MacrosTest, NarrowStringLiteral)
{
    EXPECT_STREQ(CASTLE_STRING(abc), "abc");
}

TEST(MacrosTest, WideStringLiteral)
{
    EXPECT_EQ(wcscmp(CASTLE_WIDE_STRING(abc), L"abc"), 0);
}

TEST(MacrosTest, Utf16StringLiteral)
{
    const char16_t* text = CASTLE_U16_STRING(ab);
    EXPECT_EQ(text[0], u'a');
    EXPECT_EQ(text[1], u'b');
    EXPECT_EQ(text[2], u'\0');
}

TEST(MacrosTest, Utf32StringLiteral)
{
    const char32_t* text = CASTLE_U32_STRING(ab);
    EXPECT_EQ(text[0], U'a');
    EXPECT_EQ(text[1], U'b');
    EXPECT_EQ(text[2], U'\0');
}

TEST(MacrosTest, BitHelpers)
{
    EXPECT_EQ(CASTLE_BIT(0), 1U);
    EXPECT_EQ(CASTLE_BIT(3), 8U);
    EXPECT_EQ(CASTLE_BIT(31), 0x80000000U);
}

TEST(MacrosTest, Bit64Helpers)
{
    EXPECT_EQ(CASTLE_BIT64(0), 1ULL);
    EXPECT_EQ(CASTLE_BIT64(63), 0x8000000000000000ULL);
}

} // namespace
