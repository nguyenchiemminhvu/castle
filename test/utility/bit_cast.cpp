#include <gtest/gtest.h>

#include "castle/utility/bit_cast.h"

#include <stdint.h>

namespace
{

TEST(BitCastTest, IntToFloatRoundTrips)
{
    const uint32_t bits = 0x3F800000U; // 1.0f
    const float f = castle::bit_cast<float>(bits);
    EXPECT_FLOAT_EQ(f, 1.0f);

    const uint32_t back = castle::bit_cast<uint32_t>(f);
    EXPECT_EQ(back, bits);
}

TEST(BitCastTest, SignedUnsignedReinterpret)
{
    const int32_t negative = -1;
    const uint32_t as_unsigned = castle::bit_cast<uint32_t>(negative);
    EXPECT_EQ(as_unsigned, 0xFFFFFFFFU);

    const int32_t back = castle::bit_cast<int32_t>(as_unsigned);
    EXPECT_EQ(back, negative);
}

TEST(BitCastTest, DoubleRoundTrip)
{
    const double value = 3.141592653589793;
    const uint64_t bits = castle::bit_cast<uint64_t>(value);
    const double back = castle::bit_cast<double>(bits);
    EXPECT_DOUBLE_EQ(back, value);
}

#if CASTLE_HAS_BUILTIN_BIT_CAST
TEST(BitCastTest, IsConstexprWhenBuiltinAvailable)
{
    constexpr uint32_t bits = 0x42280000U; // 42.0f
    constexpr float f = castle::bit_cast<float>(bits);
    static_assert(f == 42.0f, "bit_cast must be constexpr with builtin");
    EXPECT_FLOAT_EQ(f, 42.0f);
}
#endif

TEST(BitCastTest, IsNoexcept)
{
    uint32_t bits = 0U;
    static_assert(noexcept(castle::bit_cast<float>(bits)),
                  "bit_cast must be noexcept");
    (void)bits;
}

} // namespace
