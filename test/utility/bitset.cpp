#include <gtest/gtest.h>

#include "castle/utility/bitset.h"

#include <stdint.h>

namespace
{

using Bits8 = castle::bitset<8>;

TEST(BitsetTest, DefaultConstructedIsAllZero)
{
    Bits8 bits;
    EXPECT_TRUE(bits.none());
    EXPECT_FALSE(bits.any());
    EXPECT_FALSE(bits.all());
    EXPECT_EQ(bits.count(), 0U);
}

TEST(BitsetTest, CapacityQueries)
{
    EXPECT_EQ(Bits8::size(), 8U);
    EXPECT_EQ(Bits8::bits_per_word(), 32U);
    EXPECT_EQ(Bits8::number_of_words(), 1U);
    EXPECT_EQ(Bits8::npos(), 8U);
}

TEST(BitsetTest, CustomWordType)
{
    using Small = castle::bitset<10, uint8_t>;
    EXPECT_EQ(Small::size(), 10U);
    EXPECT_EQ(Small::bits_per_word(), 8U);
    EXPECT_EQ(Small::number_of_words(), 2U);
}

TEST(BitsetTest, SetAndTest)
{
    Bits8 bits;
    bits.set(2);
    EXPECT_TRUE(bits.test(2));
    EXPECT_FALSE(bits.test(3));
    EXPECT_EQ(bits.count(), 1U);
    EXPECT_TRUE(bits.any());
    EXPECT_FALSE(bits.none());
}

TEST(BitsetTest, SetWithValue)
{
    Bits8 bits;
    bits.set(1, true);
    EXPECT_TRUE(bits.test(1));
    bits.set(1, false);
    EXPECT_FALSE(bits.test(1));
}

TEST(BitsetTest, SetAllThenReset)
{
    Bits8 bits;
    bits.set();
    EXPECT_TRUE(bits.all());
    EXPECT_EQ(bits.count(), 8U);
    bits.reset();
    EXPECT_TRUE(bits.none());
}

TEST(BitsetTest, ResetSingleBit)
{
    Bits8 bits;
    bits.set();
    bits.reset(4);
    EXPECT_FALSE(bits.test(4));
    EXPECT_EQ(bits.count(), 7U);
}

TEST(BitsetTest, FlipAllAndSingle)
{
    Bits8 bits;
    bits.flip();
    EXPECT_TRUE(bits.all());
    bits.flip(0);
    EXPECT_FALSE(bits.test(0));
    EXPECT_EQ(bits.count(), 7U);
}

TEST(BitsetTest, OutOfRangeAccessIsSafe)
{
    Bits8 bits;
    bits.set(100);
    EXPECT_FALSE(bits.test(100));
    EXPECT_TRUE(bits.none());
}

TEST(BitsetTest, AssignFromInteger)
{
    Bits8 bits;
    bits.assign(0xA5U);
    EXPECT_EQ(bits.to_ulong(), 0xA5UL);
    EXPECT_TRUE(bits.test(0));
    EXPECT_TRUE(bits.test(7));
    EXPECT_FALSE(bits.test(1));
}

TEST(BitsetTest, ConstructFromInteger)
{
    Bits8 bits(0x0FU);
    EXPECT_EQ(bits.to_ulong(), 0x0FUL);
    EXPECT_EQ(bits.count(), 4U);
}

TEST(BitsetTest, ToUllong)
{
    castle::bitset<64> bits;
    bits.assign(0x1122334455667788ULL);
    EXPECT_EQ(bits.to_ullong(), 0x1122334455667788ULL);
}

TEST(BitsetTest, FromStringRoundTrip)
{
    castle::bitset<4> bits("1010");
    EXPECT_EQ(bits.to_ulong(), 10UL);
    EXPECT_TRUE(bits.test(1));
    EXPECT_TRUE(bits.test(3));
}

TEST(BitsetTest, ToString)
{
    Bits8 bits;
    bits.assign(0xA5U);
    char buffer[9] = {0};
    EXPECT_TRUE(bits.to_string(buffer, sizeof(buffer)));
    EXPECT_STREQ(buffer, "10100101");
}

TEST(BitsetTest, ToStringFailsWhenBufferTooSmall)
{
    Bits8 bits;
    char buffer[4] = {0};
    EXPECT_FALSE(bits.to_string(buffer, sizeof(buffer)));
}

TEST(BitsetTest, FindFirstAndNext)
{
    Bits8 bits;
    bits.set(2);
    bits.set(5);
    EXPECT_EQ(bits.find_first(true), 2U);
    EXPECT_EQ(bits.find_next(2, true), 5U);
    EXPECT_EQ(bits.find_next(5, true), Bits8::npos());
    EXPECT_EQ(bits.find_first(false), 0U);
}

TEST(BitsetTest, ReferenceProxyAssignment)
{
    Bits8 bits;
    bits[3] = true;
    EXPECT_TRUE(bits.test(3));
    EXPECT_TRUE(static_cast<bool>(bits[3]));
    EXPECT_TRUE(~bits[0]);
    bits[3] = false;
    EXPECT_FALSE(bits.test(3));
}

TEST(BitsetTest, ReferenceProxyFlip)
{
    Bits8 bits;
    bits[2].flip();
    EXPECT_TRUE(bits.test(2));
}

TEST(BitsetTest, ConstIndexReturnsBool)
{
    Bits8 bits;
    bits.set(1);
    const Bits8& ref = bits;
    EXPECT_TRUE(ref[1]);
    EXPECT_FALSE(ref[0]);
}

TEST(BitsetTest, CompoundBitwiseAnd)
{
    Bits8 a(0xF0U);
    Bits8 b(0x3CU);
    a &= b;
    EXPECT_EQ(a.to_ulong(), 0x30UL);
}

TEST(BitsetTest, CompoundBitwiseOr)
{
    Bits8 a(0xF0U);
    Bits8 b(0x0FU);
    a |= b;
    EXPECT_EQ(a.to_ulong(), 0xFFUL);
}

TEST(BitsetTest, CompoundBitwiseXor)
{
    Bits8 a(0xFFU);
    Bits8 b(0x0FU);
    a ^= b;
    EXPECT_EQ(a.to_ulong(), 0xF0UL);
}

TEST(BitsetTest, NonMemberBitwiseOperators)
{
    Bits8 a(0xF0U);
    Bits8 b(0x0FU);
    EXPECT_EQ((a & b).to_ulong(), 0x00UL);
    EXPECT_EQ((a | b).to_ulong(), 0xFFUL);
    EXPECT_EQ((a ^ b).to_ulong(), 0xFFUL);
    EXPECT_EQ((~a).to_ulong(), 0x0FUL);
}

TEST(BitsetTest, LeftShift)
{
    Bits8 bits(0x01U);
    bits <<= 3;
    EXPECT_EQ(bits.to_ulong(), 0x08UL);
    EXPECT_EQ((Bits8(0x01U) << 4).to_ulong(), 0x10UL);
}

TEST(BitsetTest, RightShift)
{
    Bits8 bits(0x80U);
    bits >>= 3;
    EXPECT_EQ(bits.to_ulong(), 0x10UL);
    EXPECT_EQ((Bits8(0x80U) >> 4).to_ulong(), 0x08UL);
}

TEST(BitsetTest, ShiftBeyondSizeClears)
{
    Bits8 bits(0xFFU);
    bits <<= 100;
    EXPECT_TRUE(bits.none());
}

TEST(BitsetTest, EqualityAndInequality)
{
    Bits8 a(0x55U);
    Bits8 b(0x55U);
    Bits8 c(0xAAU);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(BitsetTest, MemberSwap)
{
    Bits8 a(0x0FU);
    Bits8 b(0xF0U);
    a.swap(b);
    EXPECT_EQ(a.to_ulong(), 0xF0UL);
    EXPECT_EQ(b.to_ulong(), 0x0FUL);
}

TEST(BitsetTest, FreeSwap)
{
    Bits8 a(0x01U);
    Bits8 b(0x02U);
    castle::swap(a, b);
    EXPECT_EQ(a.to_ulong(), 0x02UL);
    EXPECT_EQ(b.to_ulong(), 0x01UL);
}

TEST(BitsetTest, CopyConstructionAndAssignment)
{
    Bits8 a(0x3CU);
    Bits8 b(a);
    EXPECT_TRUE(a == b);
    Bits8 c;
    c = a;
    EXPECT_TRUE(a == c);
}

TEST(BitsetTest, NonMultipleOfWordAllOnes)
{
    castle::bitset<10, uint8_t> bits;
    bits.set();
    EXPECT_TRUE(bits.all());
    EXPECT_EQ(bits.count(), 10U);
    EXPECT_EQ(bits.to_ulong(), 0x3FFUL);
}

TEST(BitsetTest, DataProvidesRawAccess)
{
    Bits8 bits(0x12U);
    EXPECT_EQ(bits.data()[0], 0x12U);
}

} // namespace
