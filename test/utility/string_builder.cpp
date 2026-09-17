#include <gtest/gtest.h>

#include "castle/utility/string_builder.hpp"

namespace
{

using builder8 = castle::string_builder<8U>;
using builder16 = castle::string_builder<16U>;
using builder32 = castle::string_builder<32U>;
using wbuilder8 = castle::wstring_builder<8U>;
using wbuilder16 = castle::wstring_builder<16U>;
using wbuilder32 = castle::wstring_builder<32U>;

TEST(StringBuilderTest, DefaultConstructedIsEmpty)
{
    builder16 sb;
    EXPECT_TRUE(sb.empty());
    EXPECT_EQ(sb.size(), 0U);
    EXPECT_EQ(sb.capacity(), 16U);
    EXPECT_FALSE(sb.truncated());
    EXPECT_EQ(sb.build_status(), castle::status::ok);
    EXPECT_STREQ(sb.c_str(), "");

    wbuilder16 wsb;
    EXPECT_TRUE(wsb.empty());
    EXPECT_EQ(wsb.size(), 0U);
    EXPECT_EQ(wsb.capacity(), 16U);
    EXPECT_FALSE(wsb.truncated());
    EXPECT_EQ(wsb.build_status(), castle::status::ok);
    EXPECT_STREQ(wsb.c_str(), L"");
}

TEST(StringBuilderTest, AppendTextAndChar)
{
    builder16 sb;
    sb.append("ab").append('c');
    EXPECT_STREQ(sb.c_str(), "abc");
    EXPECT_EQ(sb.size(), 3U);

    wbuilder16 wsb;
    wsb.append(L"ab").append(L'c');
    EXPECT_STREQ(wsb.c_str(), L"abc");
    EXPECT_EQ(wsb.size(), 3U);
}

TEST(StringBuilderTest, AppendBool)
{
    builder16 sb;
    sb.append(true).append(' ').append(false);
    EXPECT_STREQ(sb.c_str(), "true false");

    wbuilder16 wsb;
    wsb.append(true).append(L' ').append(false);
    EXPECT_STREQ(wsb.c_str(), L"true false");
}

TEST(StringBuilderTest, AppendSignedAndUnsignedIntegers)
{
    builder32 sb;
    sb.append(-42).append(' ').append(7U).append(' ').append(static_cast<int64_t>(-9000000000LL));
    EXPECT_STREQ(sb.c_str(), "-42 7 -9000000000");

    wbuilder32 wsb;
    wsb.append(-42).append(L' ').append(7U).append(L' ').append(static_cast<int64_t>(-9000000000LL));
    EXPECT_STREQ(wsb.c_str(), L"-42 7 -9000000000");
}

TEST(StringBuilderTest, AppendZero)
{
    builder16 sb;
    sb.append(0);
    EXPECT_STREQ(sb.c_str(), "0");

    wbuilder16 wsb;
    wsb.append(0);
    EXPECT_STREQ(wsb.c_str(), L"0");
}

TEST(StringBuilderTest, AppendFloatingPointDefaultPrecision)
{
    builder16 sb;
    sb.append(25.3F);
    EXPECT_STREQ(sb.c_str(), "25.300");

    wbuilder16 wsb;
    wsb.append(25.3F);
    EXPECT_STREQ(wsb.c_str(), L"25.300");
}

TEST(StringBuilderTest, AppendFloatingPointCustomPrecisionAndRounding)
{
    builder16 sb;
    sb.append(3.14159, 2U);
    EXPECT_STREQ(sb.c_str(), "3.14");

    wbuilder16 wsb;
    wsb.append(3.14159, 2U);
    EXPECT_STREQ(wsb.c_str(), L"3.14");
}

TEST(StringBuilderTest, AppendFloatingPointRoundsUpCarry)
{
    builder16 sb;
    sb.append(1.999, 2U);
    EXPECT_STREQ(sb.c_str(), "2.00");

    wbuilder16 wsb;
    wsb.append(1.999, 2U);
    EXPECT_STREQ(wsb.c_str(), L"2.00");
}

TEST(StringBuilderTest, AppendNegativeFloatingPoint)
{
    builder16 sb;
    sb.append(-2.5, 1U);
    EXPECT_STREQ(sb.c_str(), "-2.5");

    wbuilder16 wsb;
    wsb.append(-2.5, 1U);
    EXPECT_STREQ(wsb.c_str(), L"-2.5");
}

TEST(StringBuilderTest, AppendNan)
{
    builder16 sb;
    double nan_value = 0.0;
    nan_value = nan_value / nan_value;
    sb.append(nan_value);
    EXPECT_STREQ(sb.c_str(), "nan");

    wbuilder16 wsb;
    double wnan_value = 0.0;
    wnan_value = wnan_value / wnan_value;
    wsb.append(wnan_value);
    EXPECT_STREQ(wsb.c_str(), L"nan");
}

TEST(StringBuilderTest, AppendInfinity)
{
    builder16 sb;
    double inf_value = 1.0;
    inf_value = inf_value / 0.0;
    sb.append(inf_value);
    EXPECT_STREQ(sb.c_str(), "inf");

    wbuilder16 wsb;
    double winf_value = 1.0;
    winf_value = winf_value / 0.0;
    wsb.append(winf_value);
    EXPECT_STREQ(wsb.c_str(), L"inf");
}

enum class sensor_state : uint8_t
{
    idle = 0U,
    running = 1U
};

enum plain_code
{
    code_ok = 5
};

TEST(StringBuilderTest, AppendScopedEnum)
{
    builder16 sb;
    sb.append(sensor_state::running);
    EXPECT_STREQ(sb.c_str(), "1");

    wbuilder16 wsb;
    wsb.append(sensor_state::running);
    EXPECT_STREQ(wsb.c_str(), L"1");
}

TEST(StringBuilderTest, AppendPlainEnum)
{
    builder16 sb;
    sb.append(code_ok);
    EXPECT_STREQ(sb.c_str(), "5");

    wbuilder16 wsb;
    wsb.append(code_ok);
    EXPECT_STREQ(wsb.c_str(), L"5");
}

TEST(StringBuilderTest, AppendCastleString)
{
    castle::container::string<8U> name("temp");
    builder16 sb;
    sb.append(name).append('=').append(1);
    EXPECT_STREQ(sb.c_str(), "temp=1");

    castle::container::basic_string<wchar_t, 8U> wname(L"temp");
    wbuilder16 wsb;
    wsb.append(wname).append(L'=').append(1);
    EXPECT_STREQ(wsb.c_str(), L"temp=1");
}

TEST(StringBuilderTest, StreamOperatorChains)
{
    builder32 sb;
    sb << "x=" << 10 << " y=" << 20;
    EXPECT_STREQ(sb.c_str(), "x=10 y=20");

    wbuilder32 wsb;
    wsb << L"x=" << 10 << L" y=" << 20;
    EXPECT_STREQ(wsb.c_str(), L"x=10 y=20");
}

TEST(StringBuilderTest, BuildFoldsMultipleArguments)
{
    builder32 sb;
    sb.build("count=", 3, " ok=", true);
    EXPECT_STREQ(sb.c_str(), "count=3 ok=true");

    wbuilder32 wsb;
    wsb.build(L"count=", 3, L" ok=", true);
    EXPECT_STREQ(wsb.c_str(), L"count=3 ok=true");
}

TEST(StringBuilderTest, ClearResetsContentAndTruncatedFlag)
{
    builder8 sb;
    sb.append("1234567890");
    EXPECT_TRUE(sb.truncated());
    sb.clear();
    EXPECT_FALSE(sb.truncated());
    EXPECT_TRUE(sb.empty());
    EXPECT_EQ(sb.build_status(), castle::status::ok);

    wbuilder8 wsb;
    wsb.append(L"1234567890");
    EXPECT_TRUE(wsb.truncated());
    wsb.clear();
    EXPECT_FALSE(wsb.truncated());
    EXPECT_TRUE(wsb.empty());
    EXPECT_EQ(wsb.build_status(), castle::status::ok);
}

TEST(StringBuilderTest, OverflowAppliesEllipsisMarker)
{
    builder8 sb;
    sb.append("1234567890");
    EXPECT_TRUE(sb.truncated());
    EXPECT_EQ(sb.build_status(), castle::status::data_loss);
    EXPECT_STREQ(sb.c_str(), "12345...");
    EXPECT_EQ(sb.size(), 8U);

    wbuilder8 wsb;
    wsb.append(L"1234567890");
    EXPECT_TRUE(wsb.truncated());
    EXPECT_EQ(wsb.build_status(), castle::status::data_loss);
    EXPECT_STREQ(wsb.c_str(), L"12345...");
    EXPECT_EQ(wsb.size(), 8U);
}

TEST(StringBuilderTest, OverflowTrimsTrailingWhitespaceBeforeEllipsisByDefault)
{
    // capacity 13: "value" + 8 trailing spaces fills the buffer exactly; the next
    // append cannot fit at all, so all trailing whitespace is trimmed first and
    // "..." is glued directly onto "value".
    castle::string_builder<13U> sb;
    sb.append("value");
    for (int i = 0; i < 8; ++i)
    {
        sb.append(' ');
    }
    sb.append("!");
    EXPECT_TRUE(sb.truncated());
    EXPECT_STREQ(sb.c_str(), "value...");

    castle::wstring_builder<13U> wsb;
    wsb.append(L"value");
    for (int i = 0; i < 8; ++i)
    {
        wsb.append(L' ');
    }
    wsb.append(L"!");
    EXPECT_TRUE(wsb.truncated());
    EXPECT_STREQ(wsb.c_str(), L"value...");
}

TEST(StringBuilderTest, OverflowKeepsWhitespaceWhenTrimmingDisabled)
{
    // Same setup, but with trimming disabled only the minimum number of
    // characters needed to fit the marker are dropped, so some of the
    // whitespace survives right before "...".
    castle::string_builder<13U, false> sb;
    sb.append("value");
    for (int i = 0; i < 8; ++i)
    {
        sb.append(' ');
    }
    sb.append("!");
    EXPECT_TRUE(sb.truncated());
    EXPECT_STREQ(sb.c_str(), "value     ...");

    castle::wstring_builder<13U, false> wsb;
    wsb.append(L"value");
    for (int i = 0; i < 8; ++i)
    {
        wsb.append(L' ');
    }
    wsb.append(L"!");
    EXPECT_TRUE(wsb.truncated());
    EXPECT_STREQ(wsb.c_str(), L"value     ...");
}

TEST(StringBuilderTest, TruncationSealsAndIgnoresFurtherAppends)
{
    builder8 sb;
    sb.append("1234567890");
    EXPECT_STREQ(sb.c_str(), "12345...");
    sb.append("more text");
    EXPECT_STREQ(sb.c_str(), "12345...");

    wbuilder8 wsb;
    wsb.append(L"1234567890");
    EXPECT_TRUE(wsb.truncated());
    EXPECT_STREQ(wsb.c_str(), L"12345...");
    wsb.append(L"more text");
    EXPECT_STREQ(wsb.c_str(), L"12345...");
}

TEST(StringBuilderTest, ExactFitDoesNotTruncate)
{
    builder8 sb;
    sb.append("12345678");
    EXPECT_FALSE(sb.truncated());
    EXPECT_STREQ(sb.c_str(), "12345678");

    wbuilder8 wsb;
    wsb.append(L"12345678");
    EXPECT_FALSE(wsb.truncated());
    EXPECT_STREQ(wsb.c_str(), L"12345678");
}

TEST(StringBuilderTest, TinyCapacityDegradesGracefully)
{
    castle::string_builder<1U> sb;
    sb.append("abcdef");
    EXPECT_TRUE(sb.truncated());
    EXPECT_EQ(sb.size(), 1U);
    EXPECT_STREQ(sb.c_str(), ".");

    castle::wstring_builder<1U> wsb;
    wsb.append(L"abcdef");
    EXPECT_TRUE(wsb.truncated());
    EXPECT_EQ(wsb.size(), 1U);
    EXPECT_STREQ(wsb.c_str(), L".");
}

TEST(StringBuilderTest, ViewReflectsBufferContent)
{
    builder16 sb;
    sb.append("abc");
    EXPECT_EQ(sb.view(), castle::container::string_view("abc"));

    wbuilder16 wsb;
    wsb.append(L"abc");
    EXPECT_EQ(wsb.view(), castle::container::basic_string_view<wchar_t>(L"abc"));
}

TEST(StringBuilderTest, WideCharBuilderSupportsBasicUsage)
{
    castle::wstring_builder<16U> sb;
    sb.append(L"n=").append(7).append(L' ');
    EXPECT_EQ(sb.view(), castle::container::basic_string_view<wchar_t>(L"n=7 "));

    castle::wstring_builder<16U> sb2;
    sb2.append(L"m=").append(42).append(L' ');
    EXPECT_EQ(sb2.view(), castle::container::basic_string_view<wchar_t>(L"m=42 "));
}

} // namespace
