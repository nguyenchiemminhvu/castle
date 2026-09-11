#include <gtest/gtest.h>

#include "castle/bit/bit_rotate.h"

#include <cstdint>

namespace cb = castle::bit;

namespace
{

// ── rotate_left ─────────────────────────────────────────────────

TEST(BitRotate, RotateLeftBasic)
{
    EXPECT_EQ(cb::rotate_left(uint8_t{0x01}, 1), uint8_t{0x02});
    EXPECT_EQ(cb::rotate_left(uint8_t{0x80}, 1), uint8_t{0x01}); // wrap MSB -> LSB
    EXPECT_EQ(cb::rotate_left(uint8_t{0x0F}, 4), uint8_t{0xF0});
    EXPECT_EQ(cb::rotate_left(uint16_t{0x8000}, 1), uint16_t{0x0001});
    EXPECT_EQ(cb::rotate_left(uint32_t{0x80000000U}, 1), uint32_t{0x00000001U});
    static_assert(cb::rotate_left(uint8_t{0x80}, 1) == uint8_t{0x01}, "rol wrap");
}

TEST(BitRotate, RotateLeftByZero)
{
    EXPECT_EQ(cb::rotate_left(uint8_t{0xA5}, 0), uint8_t{0xA5});
    EXPECT_EQ(cb::rotate_left(uint32_t{0x12345678U}, 0), uint32_t{0x12345678U});
}

TEST(BitRotate, RotateLeftByWidth)
{
    EXPECT_EQ(cb::rotate_left(uint8_t{0xA5}, 8), uint8_t{0xA5});
    EXPECT_EQ(cb::rotate_left(uint16_t{0x1234}, 16), uint16_t{0x1234});
    EXPECT_EQ(cb::rotate_left(uint32_t{0x12345678U}, 32), uint32_t{0x12345678U});
}

TEST(BitRotate, RotateLeftMoreThanWidth)
{
    // shift is taken modulo bit width
    EXPECT_EQ(cb::rotate_left(uint8_t{0x01}, 9), cb::rotate_left(uint8_t{0x01}, 1));
    EXPECT_EQ(cb::rotate_left(uint8_t{0x01}, 9), uint8_t{0x02});
    EXPECT_EQ(cb::rotate_left(uint32_t{0x1U}, 33), uint32_t{0x2U});
}

// ── rotate_right ────────────────────────────────────────────────

TEST(BitRotate, RotateRightBasic)
{
    EXPECT_EQ(cb::rotate_right(uint8_t{0x01}, 1), uint8_t{0x80}); // wrap LSB -> MSB
    EXPECT_EQ(cb::rotate_right(uint8_t{0x80}, 1), uint8_t{0x40});
    EXPECT_EQ(cb::rotate_right(uint8_t{0xF0}, 4), uint8_t{0x0F});
    EXPECT_EQ(cb::rotate_right(uint16_t{0x0001}, 1), uint16_t{0x8000});
    EXPECT_EQ(cb::rotate_right(uint32_t{0x00000001U}, 1), uint32_t{0x80000000U});
    static_assert(cb::rotate_right(uint8_t{0x01}, 1) == uint8_t{0x80}, "ror wrap");
}

TEST(BitRotate, RotateRightByZero)
{
    EXPECT_EQ(cb::rotate_right(uint8_t{0xA5}, 0), uint8_t{0xA5});
    EXPECT_EQ(cb::rotate_right(uint32_t{0x12345678U}, 0), uint32_t{0x12345678U});
}

TEST(BitRotate, RotateRightByWidth)
{
    EXPECT_EQ(cb::rotate_right(uint8_t{0xA5}, 8), uint8_t{0xA5});
    EXPECT_EQ(cb::rotate_right(uint32_t{0x12345678U}, 32), uint32_t{0x12345678U});
}

TEST(BitRotate, RotateRightMoreThanWidth)
{
    EXPECT_EQ(cb::rotate_right(uint8_t{0x01}, 9), cb::rotate_right(uint8_t{0x01}, 1));
    EXPECT_EQ(cb::rotate_right(uint8_t{0x01}, 9), uint8_t{0x80});
}

// ── round trips ─────────────────────────────────────────────────

TEST(BitRotate, LeftRightRoundTrip)
{
    const uint32_t v = 0xDEADBEEFU;
    EXPECT_EQ(cb::rotate_right(cb::rotate_left(v, 7), 7), v);
    EXPECT_EQ(cb::rotate_left(cb::rotate_right(v, 13), 13), v);

    const uint8_t b = 0xC3;
    EXPECT_EQ(cb::rotate_right(cb::rotate_left(b, 3), 3), b);
}

} // namespace
