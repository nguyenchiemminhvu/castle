#include <gtest/gtest.h>

#include "castle/bit/bit.h"

#include <cstdint>

// Umbrella header: verify each sub-module's public API is reachable
// through the single include and behaves consistently.

namespace cb = castle::bit;

namespace
{

TEST(BitUmbrella, CorePresent)
{
    EXPECT_TRUE(cb::test(uint8_t{0x08}, 3));
    EXPECT_EQ(cb::set(uint8_t{0}, 3), uint8_t{0x08});
    EXPECT_EQ(cb::clear(uint8_t{0xFF}, 0), uint8_t{0xFE});
    EXPECT_EQ(cb::toggle(uint8_t{0}, 0), uint8_t{0x01});
}

TEST(BitUmbrella, CountPresent)
{
    EXPECT_EQ(cb::popcount(uint8_t{0xFF}), 8u);
    EXPECT_EQ(cb::count_leading_zeros(uint8_t{0x01}), 7u);
    EXPECT_EQ(cb::count_trailing_zeros(uint8_t{0x80}), 7u);
    EXPECT_EQ(cb::bit_width(uint8_t{0x05}), 3u);
}

TEST(BitUmbrella, MaskPresent)
{
    EXPECT_EQ(cb::all_bits_mask<uint8_t>::value, uint8_t{0xFF});
    EXPECT_EQ(cb::low_bits_mask<uint8_t>(3), uint8_t{0x07});
    EXPECT_EQ(cb::high_bits_mask<uint8_t>(3), uint8_t{0xE0});
}

TEST(BitUmbrella, MathPresent)
{
    EXPECT_TRUE(cb::is_power_of_two(uint8_t{16}));
    EXPECT_EQ(cb::next_power_of_two(uint32_t{5}), uint32_t{8});
    EXPECT_EQ(cb::align_up(uint32_t{5}, uint32_t{4}), uint32_t{8});
}

TEST(BitUmbrella, ReversePresent)
{
    EXPECT_EQ(cb::reverse_bits(uint8_t{0x01}), uint8_t{0x80});
    EXPECT_EQ(cb::byte_swap(uint16_t{0x1234}), uint16_t{0x3412});
}

TEST(BitUmbrella, RotatePresent)
{
    EXPECT_EQ(cb::rotate_left(uint8_t{0x80}, 1), uint8_t{0x01});
    EXPECT_EQ(cb::rotate_right(uint8_t{0x01}, 1), uint8_t{0x80});
}

TEST(BitUmbrella, UtilsPresent)
{
    EXPECT_EQ(cb::extract_lowest_set_bit(uint8_t{0xB4}), uint8_t{0x04});
    EXPECT_EQ(cb::extract_field(uint8_t{0xB4}, 2, 3), uint8_t{0x05});
}

TEST(BitUmbrella, FlagsPresent)
{
    cb::flags<uint8_t> f;
    f.set(0x05);
    EXPECT_EQ(f.value(), uint8_t{0x05});
    EXPECT_TRUE(f.any());
}

} // namespace
