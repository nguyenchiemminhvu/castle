#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/bit_writer.hpp"
#include "castle_ext/protocols/rtcm/messages/station_arp.hpp"

namespace
{

using namespace castle::protocols::rtcm;
using castle::container::array_view;

/** @brief Message 1005 golden frame, decoded and CRC-checked with pyrtcm. */
const uint8_t golden_1005[25U] = {
    0xD3U, 0x00U, 0x13U, 0x3EU, 0xD4U, 0xD2U, 0x3BU, 0x09U, 0x60U, 0xCFU, 0xD9U, 0x9CU, 0x80U,
    0xB7U, 0x03U, 0x70U, 0xC0U, 0x8BU, 0x74U, 0x3CU, 0x4CU, 0xACU, 0x19U, 0xD1U, 0xF1U};

template <size_t N>
void fill(uint8_t (&buffer)[N], uint8_t value)
{
    for (uint8_t& byte : buffer)
    {
        byte = value;
    }
}

template <size_t N>
bit_writer make_writer(uint8_t (&buffer)[N])
{
    return bit_writer(array_view<uint8_t>(buffer, N));
}

// -----------------------------------------------------------------------------
// Local message models for encode_message()
// -----------------------------------------------------------------------------

/** @brief 12 + 4 + 3 bits: payload AB CD A0. */
struct simple_message
{
    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.field(static_cast<uint16_t>(0xABCU), 12U);
        writer.field(static_cast<uint8_t>(0xDU), 4U);
        writer.field(static_cast<uint8_t>(0x5U), 3U);
    }
};

/** @brief Writes nothing at all. */
struct empty_message
{
    void encode(bit_writer&) CASTLE_CONST CASTLE_NOEXCEPT {}
};

/** @brief Writes `bytes` zero bytes. */
struct zero_bytes_message
{
    castle::size_type bytes;

    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.reserved(bytes * 8U);
    }
};

/** @brief Latches an encoder failure after writing some data. */
struct failing_message
{
    castle::status reason;

    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.field(static_cast<uint8_t>(0x11U), 8U);
        writer.fail(reason);
        writer.field(static_cast<uint8_t>(0x22U), 8U);
    }
};

/** @brief Writes a value that does not fit its field. */
struct too_wide_message
{
    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.field(static_cast<uint16_t>(0x1000U), 12U);
    }
};

/** @brief Records the capacity the writer was given. */
struct probe_message
{
    mutable castle::size_type remaining = 0U;

    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        remaining = writer.remaining_bits();
    }
};

// -----------------------------------------------------------------------------
// write_bits
// -----------------------------------------------------------------------------

TEST(RtcmBitWriter, FreshWriterReportsEmptyState)
{
    uint8_t buffer[4U] = {};
    bit_writer writer = make_writer(buffer);
    EXPECT_TRUE(writer.ok());
    EXPECT_EQ(writer.status(), castle::status::ok);
    EXPECT_EQ(writer.bit_size(), 0U);
    EXPECT_EQ(writer.size(), 0U);
    EXPECT_EQ(writer.remaining_bits(), 32U);
}

TEST(RtcmBitWriter, PacksMostSignificantBitFirstAcrossByteBoundaries)
{
    uint8_t buffer[4U] = {};
    bit_writer writer = make_writer(buffer);

    writer.write_bits(0x5U, 3U);   // 101
    writer.write_bits(0xFFU, 8U);  // 11111111
    writer.write_bits(0x1U, 1U);   // 1
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 12U);
    EXPECT_EQ(writer.size(), 2U);
    EXPECT_EQ(writer.remaining_bits(), 20U);
    EXPECT_EQ(buffer[0U], 0xBFU); // 1011 1111
    EXPECT_EQ(buffer[1U], 0xF0U); // 1111 + zero padding
    EXPECT_EQ(buffer[2U], 0x00U);
}

TEST(RtcmBitWriter, WritesWholeBytesAtByteBoundary)
{
    uint8_t buffer[3U] = {};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0x12U, 8U);
    writer.write_bits(0x3456U, 16U);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.size(), 3U);
    EXPECT_EQ(writer.remaining_bits(), 0U);
    EXPECT_EQ(buffer[0U], 0x12U);
    EXPECT_EQ(buffer[1U], 0x34U);
    EXPECT_EQ(buffer[2U], 0x56U);
}

TEST(RtcmBitWriter, ZeroWidthWritesNothing)
{
    uint8_t buffer[2U] = {0xEEU, 0xEEU};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0U, 0U);
    EXPECT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 0U);
    EXPECT_EQ(buffer[0U], 0xEEU);

    writer.write_bits(0x3U, 2U);
    writer.write_bits(0U, 0U);
    EXPECT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 2U);
}

TEST(RtcmBitWriter, NonZeroValueInZeroWidthIsOutOfRange)
{
    uint8_t buffer[2U] = {};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(1U, 0U);
    EXPECT_FALSE(writer.ok());
    EXPECT_EQ(writer.status(), castle::status::out_of_range);
    EXPECT_EQ(writer.bit_size(), 0U);
}

TEST(RtcmBitWriter, SingleBitAndSevenBitWidths)
{
    uint8_t buffer[2U] = {};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(1U, 1U);
    writer.write_bits(0x7FU, 7U);
    writer.write_bits(0U, 1U);
    writer.write_bits(0x55U, 7U);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 16U);
    EXPECT_EQ(buffer[0U], 0xFFU);
    EXPECT_EQ(buffer[1U], 0x55U);
}

TEST(RtcmBitWriter, EightBitFieldAtEveryBitOffset)
{
    for (castle::size_type offset = 0U; offset < 8U; ++offset)
    {
        uint8_t buffer[3U] = {0xFFU, 0xFFU, 0xFFU};
        bit_writer writer = make_writer(buffer);
        writer.write_bits(0U, offset);
        writer.write_bits(0xA7U, 8U);
        ASSERT_TRUE(writer.ok());
        EXPECT_EQ(writer.bit_size(), offset + 8U);

        const uint16_t joined = static_cast<uint16_t>((static_cast<uint16_t>(buffer[0U]) << 8U) | buffer[1U]);
        EXPECT_EQ(static_cast<uint8_t>(joined >> (8U - offset)), 0xA7U) << "offset " << offset;
    }
}

TEST(RtcmBitWriter, NineBitFieldSpansTwoBytes)
{
    uint8_t buffer[3U] = {};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0U, 7U);
    writer.write_bits(0x1FFU, 9U); // bits 7..15
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(buffer[0U], 0x01U);
    EXPECT_EQ(buffer[1U], 0xFFU);

    uint8_t other[3U] = {};
    bit_writer second = make_writer(other);
    second.write_bits(0x1A5U, 9U);
    ASSERT_TRUE(second.ok());
    EXPECT_EQ(other[0U], 0xD2U); // 1101 0010
    EXPECT_EQ(other[1U], 0x80U); // 1 + padding
}

TEST(RtcmBitWriter, SixtyThreeBitField)
{
    uint8_t buffer[8U] = {};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0x7FFFFFFFFFFFFFFFULL, 63U);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 63U);
    EXPECT_EQ(writer.size(), 8U);
    EXPECT_EQ(writer.remaining_bits(), 1U);
    for (size_t i = 0U; i < 7U; ++i)
    {
        EXPECT_EQ(buffer[i], 0xFFU);
    }
    EXPECT_EQ(buffer[7U], 0xFEU);

    // One over the 63-bit maximum.
    uint8_t other[16U] = {};
    bit_writer second = make_writer(other);
    second.write_bits(0x8000000000000000ULL, 63U);
    EXPECT_EQ(second.status(), castle::status::out_of_range);
    EXPECT_EQ(second.bit_size(), 0U);
}

TEST(RtcmBitWriter, SixtyFourBitFieldIsAlwaysInRange)
{
    uint8_t buffer[8U] = {};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0xFEDCBA9876543210ULL, 64U);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 64U);
    EXPECT_EQ(writer.remaining_bits(), 0U);
    const uint8_t expected[8U] = {0xFEU, 0xDCU, 0xBAU, 0x98U, 0x76U, 0x54U, 0x32U, 0x10U};
    for (size_t i = 0U; i < 8U; ++i)
    {
        EXPECT_EQ(buffer[i], expected[i]) << "byte " << i;
    }

    // Maximum value and unaligned start (spans nine bytes).
    uint8_t wide[9U] = {};
    bit_writer unaligned = make_writer(wide);
    unaligned.write_bits(0x5U, 3U);
    unaligned.write_bits(~static_cast<uint64_t>(0), 64U);
    ASSERT_TRUE(unaligned.ok());
    EXPECT_EQ(unaligned.bit_size(), 67U);
    EXPECT_EQ(wide[0U], 0xBFU); // 101 + 11111
    for (size_t i = 1U; i < 8U; ++i)
    {
        EXPECT_EQ(wide[i], 0xFFU);
    }
    EXPECT_EQ(wide[8U], 0xE0U); // 111 + padding
}

TEST(RtcmBitWriter, ValueTooWideForFieldIsOutOfRange)
{
    uint8_t buffer[4U] = {0xEEU, 0xEEU, 0xEEU, 0xEEU};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0xFU, 4U);
    writer.write_bits(0x10U, 4U); // 5 significant bits
    EXPECT_FALSE(writer.ok());
    EXPECT_EQ(writer.status(), castle::status::out_of_range);
    EXPECT_EQ(writer.bit_size(), 4U);
    EXPECT_EQ(buffer[1U], 0xEEU);

    uint8_t byte_buffer[2U] = {};
    bit_writer maximum = make_writer(byte_buffer);
    maximum.write_bits(0xFFU, 8U); // largest 8-bit value is accepted
    EXPECT_TRUE(maximum.ok());
    maximum.write_bits(0x100U, 8U); // one over
    EXPECT_EQ(maximum.status(), castle::status::out_of_range);
}

TEST(RtcmBitWriter, WidthAboveSixtyFourIsOutOfRange)
{
    uint8_t buffer[32U] = {};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0U, 65U);
    EXPECT_EQ(writer.status(), castle::status::out_of_range);
    EXPECT_EQ(writer.bit_size(), 0U);

    bit_writer hundred = make_writer(buffer);
    hundred.write_bits(1U, 100U);
    EXPECT_EQ(hundred.status(), castle::status::out_of_range);
}

TEST(RtcmBitWriter, OverflowLatchesFullWithoutPartialWrite)
{
    uint8_t buffer[2U] = {0xAAU, 0xAAU};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0x7FFU, 11U);
    writer.write_bits(0x3FU, 6U); // needs 6 bits, only 5 left
    EXPECT_FALSE(writer.ok());
    EXPECT_EQ(writer.status(), castle::status::full);
    EXPECT_EQ(writer.bit_size(), 11U);
    EXPECT_EQ(writer.remaining_bits(), 5U);
    EXPECT_EQ(buffer[0U], 0xFFU);
    EXPECT_EQ(buffer[1U], 0xE0U);
}

TEST(RtcmBitWriter, ExactFitSucceedsAndNextBitOverflows)
{
    uint8_t buffer[2U] = {};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0x1234U, 16U);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.remaining_bits(), 0U);
    EXPECT_EQ(buffer[0U], 0x12U);
    EXPECT_EQ(buffer[1U], 0x34U);

    writer.write_bits(0U, 0U); // zero-width write needs no room
    EXPECT_TRUE(writer.ok());

    writer.write_bits(1U, 1U);
    EXPECT_EQ(writer.status(), castle::status::full);
    EXPECT_EQ(writer.bit_size(), 16U);
}

TEST(RtcmBitWriter, ErrorIsStickyAndFirstFailureWins)
{
    uint8_t buffer[4U] = {0xAAU, 0xAAU, 0xAAU, 0xAAU};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0x2U, 1U); // out_of_range first
    ASSERT_EQ(writer.status(), castle::status::out_of_range);

    writer.write_bits(0x1U, 1U); // would fit, but the error is sticky
    writer.write_bits(0xFFU, 100U);
    writer.field(static_cast<uint8_t>(1U), 8U);
    writer.field(true);
    writer.reserved(8U);
    EXPECT_EQ(writer.status(), castle::status::out_of_range); // not replaced by a later failure
    EXPECT_EQ(writer.bit_size(), 0U);
    EXPECT_EQ(buffer[0U], 0xAAU);
}

TEST(RtcmBitWriter, FailKeepsOnlyFirstReason)
{
    uint8_t buffer[4U] = {};
    bit_writer writer = make_writer(buffer);
    EXPECT_EQ(writer.status(), castle::status::ok);

    writer.fail(castle::status::invalid_argument);
    writer.fail(castle::status::out_of_range);
    writer.fail(castle::status::full);
    EXPECT_FALSE(writer.ok());
    EXPECT_EQ(writer.status(), castle::status::invalid_argument);

    writer.write_bits(1U, 1U);
    EXPECT_EQ(writer.bit_size(), 0U);
}

TEST(RtcmBitWriter, FailAfterOverflowKeepsFull)
{
    uint8_t buffer[1U] = {};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0U, 9U);
    ASSERT_EQ(writer.status(), castle::status::full);
    writer.fail(castle::status::invalid_argument);
    EXPECT_EQ(writer.status(), castle::status::full);
}

TEST(RtcmBitWriter, NullViewBehavesAsZeroCapacity)
{
    bit_writer empty{array_view<uint8_t>{}};
    EXPECT_TRUE(empty.ok());
    EXPECT_EQ(empty.remaining_bits(), 0U);
    EXPECT_EQ(empty.size(), 0U);
    empty.write_bits(0U, 0U);
    EXPECT_TRUE(empty.ok());
    empty.write_bits(1U, 1U);
    EXPECT_EQ(empty.status(), castle::status::full);

    bit_writer dangling{array_view<uint8_t>(nullptr, 8U)};
    EXPECT_EQ(dangling.remaining_bits(), 0U);
    dangling.field(true);
    EXPECT_EQ(dangling.status(), castle::status::full);
}

TEST(RtcmBitWriter, SizeRoundsUpToWholeBytes)
{
    uint8_t buffer[4U] = {};
    bit_writer writer = make_writer(buffer);
    EXPECT_EQ(writer.size(), 0U);
    writer.write_bits(1U, 1U);
    EXPECT_EQ(writer.size(), 1U);
    writer.write_bits(0x7FU, 7U);
    EXPECT_EQ(writer.bit_size(), 8U);
    EXPECT_EQ(writer.size(), 1U);
    writer.write_bits(0U, 1U);
    EXPECT_EQ(writer.size(), 2U);
    EXPECT_EQ(writer.remaining_bits(), 23U);
}

TEST(RtcmBitWriter, TrailingPadBitsAreZeroEvenOverDirtyBuffer)
{
    uint8_t buffer[4U];
    fill(buffer, 0xFFU);
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0x5U, 3U);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(buffer[0U], 0xA0U); // 101 + five zero pad bits
    EXPECT_EQ(buffer[1U], 0xFFU); // untouched, outside the payload

    fill(buffer, 0xFFU);
    bit_writer twelve = make_writer(buffer);
    twelve.write_bits(0xFFFU, 12U);
    EXPECT_EQ(buffer[0U], 0xFFU);
    EXPECT_EQ(buffer[1U], 0xF0U);
    EXPECT_EQ(twelve.size(), 2U);

    // A field starting mid-byte must not clear earlier bits of that byte.
    fill(buffer, 0xFFU);
    bit_writer mid = make_writer(buffer);
    mid.write_bits(0x2U, 2U);
    mid.write_bits(0x1U, 2U);
    EXPECT_EQ(buffer[0U], 0x90U); // 10 01 0000
}

// -----------------------------------------------------------------------------
// field(), reserved(), text()
// -----------------------------------------------------------------------------

TEST(RtcmBitWriter, UnsignedFieldAcceptsFullRangeOfEachWidth)
{
    uint8_t buffer[16U] = {};
    bit_writer writer = make_writer(buffer);
    writer.field(static_cast<uint8_t>(0xFFU), 8U);
    writer.field(static_cast<uint16_t>(0xFFFU), 12U);
    writer.field(static_cast<uint32_t>(0xFFFFFFFFU), 32U);
    writer.field(static_cast<uint64_t>(0x1ULL), 1U);
    writer.field(static_cast<uint16_t>(0U), 12U);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 65U);
    EXPECT_EQ(buffer[0U], 0xFFU);
    EXPECT_EQ(buffer[1U], 0xFFU);
    EXPECT_EQ(buffer[2U], 0xFFU);
    EXPECT_EQ(buffer[5U], 0xFFU);
    EXPECT_EQ(buffer[6U], 0xF8U); // F, then bit 1, then 12 zero bits start
    EXPECT_EQ(buffer[7U], 0x00U);
    EXPECT_EQ(buffer[8U], 0x00U);
}

TEST(RtcmBitWriter, UnsignedFieldRejectsValueOneOverMaximum)
{
    const castle::size_type widths[] = {1U, 2U, 6U, 12U, 16U, 38U};
    for (castle::size_type width : widths)
    {
        uint8_t buffer[8U] = {};
        bit_writer writer = make_writer(buffer);
        writer.field(static_cast<uint64_t>(1ULL << width), width);
        EXPECT_EQ(writer.status(), castle::status::out_of_range) << "width " << width;
        EXPECT_EQ(writer.bit_size(), 0U);

        bit_writer maximum = make_writer(buffer);
        maximum.field(static_cast<uint64_t>((1ULL << width) - 1ULL), width);
        EXPECT_TRUE(maximum.ok()) << "width " << width;
        EXPECT_EQ(maximum.bit_size(), width);
    }
}

TEST(RtcmBitWriter, UnsignedFieldSixtyFourBitsAndBeyond)
{
    uint8_t buffer[8U] = {};
    bit_writer writer = make_writer(buffer);
    writer.field(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL), 64U);
    EXPECT_TRUE(writer.ok());
    EXPECT_EQ(buffer[0U], 0xFFU);
    EXPECT_EQ(buffer[7U], 0xFFU);

    uint8_t wide[16U] = {};
    bit_writer over = make_writer(wide);
    over.field(static_cast<uint64_t>(1U), 65U);
    EXPECT_EQ(over.status(), castle::status::out_of_range);
}

TEST(RtcmBitWriter, UnsignedFieldWithZeroBits)
{
    uint8_t buffer[2U] = {};
    bit_writer zero = make_writer(buffer);
    zero.field(static_cast<uint8_t>(0U), 0U);
    EXPECT_TRUE(zero.ok());
    EXPECT_EQ(zero.bit_size(), 0U);

    bit_writer nonzero = make_writer(buffer);
    nonzero.field(static_cast<uint8_t>(1U), 0U);
    EXPECT_EQ(nonzero.status(), castle::status::out_of_range);
}

TEST(RtcmBitWriter, SignedFieldTwosComplementBoundaries)
{
    struct signed_case
    {
        int64_t value;
        castle::size_type bits;
        uint64_t expected;
    };
    const signed_case cases[] = {
        {0, 1U, 0x0U},
        {-1, 1U, 0x1U},
        {1, 2U, 0x1U},
        {-2, 2U, 0x2U},
        {-1, 8U, 0xFFU},
        {127, 8U, 0x7FU},
        {-128, 8U, 0x80U},
        {2047, 12U, 0x7FFU},
        {-2048, 12U, 0x800U},
        {-2, 12U, 0xFFEU},
        {137438953471LL, 38U, 0x1FFFFFFFFFULL},   // 2^37 - 1
        {-137438953472LL, 38U, 0x2000000000ULL},  // -2^37
        {-1, 38U, 0x3FFFFFFFFFULL},
        {INT64_C(0x3FFFFFFFFFFFFFFF), 63U, 0x3FFFFFFFFFFFFFFFULL},
        {-INT64_C(0x4000000000000000), 63U, 0x4000000000000000ULL},
        {INT64_MAX, 64U, 0x7FFFFFFFFFFFFFFFULL},
        {INT64_MIN, 64U, 0x8000000000000000ULL},
        {-1, 64U, 0xFFFFFFFFFFFFFFFFULL},
    };

    for (const signed_case& test_case : cases)
    {
        uint8_t buffer[8U] = {};
        bit_writer writer = make_writer(buffer);
        writer.field(test_case.value, test_case.bits);
        ASSERT_TRUE(writer.ok()) << "value " << test_case.value << " bits " << test_case.bits;
        ASSERT_EQ(writer.bit_size(), test_case.bits);

        // Rebuild the written bit pattern MSB first and compare with the two's complement value.
        uint64_t rebuilt = 0U;
        for (castle::size_type bit = 0U; bit < test_case.bits; ++bit)
        {
            rebuilt = (rebuilt << 1U) | ((buffer[bit >> 3U] >> (7U - (bit & 7U))) & 1U);
        }
        EXPECT_EQ(rebuilt, test_case.expected) << "value " << test_case.value << " bits " << test_case.bits;
    }
}

TEST(RtcmBitWriter, SignedFieldRejectsOneOverEitherLimit)
{
    struct signed_limit
    {
        castle::size_type bits;
        int64_t max;
        int64_t min;
    };
    const signed_limit limits[] = {
        {1U, 0, -1},
        {2U, 1, -2},
        {8U, 127, -128},
        {12U, 2047, -2048},
        {38U, 137438953471LL, -137438953472LL},
        {63U, INT64_C(0x3FFFFFFFFFFFFFFF), -INT64_C(0x4000000000000000)},
    };

    for (const signed_limit& limit : limits)
    {
        uint8_t buffer[8U] = {};

        bit_writer over_max = make_writer(buffer);
        over_max.field(limit.max + 1, limit.bits);
        EXPECT_EQ(over_max.status(), castle::status::out_of_range) << "bits " << limit.bits;
        EXPECT_EQ(over_max.bit_size(), 0U);

        bit_writer under_min = make_writer(buffer);
        under_min.field(limit.min - 1, limit.bits);
        EXPECT_EQ(under_min.status(), castle::status::out_of_range) << "bits " << limit.bits;
        EXPECT_EQ(under_min.bit_size(), 0U);

        bit_writer at_max = make_writer(buffer);
        at_max.field(limit.max, limit.bits);
        EXPECT_TRUE(at_max.ok()) << "bits " << limit.bits;

        bit_writer at_min = make_writer(buffer);
        at_min.field(limit.min, limit.bits);
        EXPECT_TRUE(at_min.ok()) << "bits " << limit.bits;
    }
}

TEST(RtcmBitWriter, SignedFieldWidthZeroOrAboveSixtyFourIsOutOfRange)
{
    uint8_t buffer[32U] = {};

    bit_writer zero = make_writer(buffer);
    zero.field(static_cast<int32_t>(0), 0U);
    EXPECT_EQ(zero.status(), castle::status::out_of_range);
    EXPECT_EQ(zero.bit_size(), 0U);

    bit_writer wide = make_writer(buffer);
    wide.field(static_cast<int32_t>(-1), 65U);
    EXPECT_EQ(wide.status(), castle::status::out_of_range);
    EXPECT_EQ(wide.bit_size(), 0U);
}

TEST(RtcmBitWriter, SignedFieldAcceptsEverySignedIntegerType)
{
    uint8_t buffer[8U] = {};
    bit_writer writer = make_writer(buffer);
    writer.field(static_cast<int8_t>(-3), 8U);
    writer.field(static_cast<int16_t>(-3), 16U);
    writer.field(static_cast<int32_t>(-3), 12U);
    writer.field(static_cast<int64_t>(-3), 28U);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 64U);
    EXPECT_EQ(buffer[0U], 0xFDU);
    EXPECT_EQ(buffer[1U], 0xFFU);
    EXPECT_EQ(buffer[2U], 0xFDU);
    EXPECT_EQ(buffer[3U], 0xFFU); // 12-bit -3 = FFD
    EXPECT_EQ(buffer[4U], 0xDFU);
    EXPECT_EQ(buffer[5U], 0xFFU); // 28-bit -3 = FFFFFFD
    EXPECT_EQ(buffer[6U], 0xFFU);
    EXPECT_EQ(buffer[7U], 0xFDU);
}

TEST(RtcmBitWriter, SignedFieldRespectsStickyErrorAndKeepsFirstReason)
{
    uint8_t buffer[1U] = {0xAAU};
    bit_writer writer = make_writer(buffer);
    writer.write_bits(0U, 9U);
    ASSERT_EQ(writer.status(), castle::status::full);

    writer.field(static_cast<int16_t>(-1), 8U);
    writer.field(static_cast<int16_t>(5000), 8U); // out of range, but full was latched first
    EXPECT_EQ(writer.status(), castle::status::full);
    EXPECT_EQ(writer.bit_size(), 0U);
    EXPECT_EQ(buffer[0U], 0xAAU);
}

TEST(RtcmBitWriter, SignedFieldOverflowLatchesFull)
{
    uint8_t buffer[1U] = {};
    bit_writer writer = make_writer(buffer);
    writer.field(static_cast<int32_t>(-1), 9U);
    EXPECT_EQ(writer.status(), castle::status::full);
    EXPECT_EQ(writer.bit_size(), 0U);
}

TEST(RtcmBitWriter, BoolFieldWritesOneBit)
{
    uint8_t buffer[1U] = {};
    bit_writer writer = make_writer(buffer);
    writer.field(true);
    writer.field(false);
    writer.field(true);
    writer.field(true);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 4U);
    EXPECT_EQ(buffer[0U], 0xB0U); // 1011 + padding

    bit_writer full = make_writer(buffer);
    full.reserved(8U);
    full.field(true);
    EXPECT_EQ(full.status(), castle::status::full);
}

TEST(RtcmBitWriter, ReservedWritesZeroBitsOverDirtyBuffer)
{
    uint8_t buffer[4U];
    fill(buffer, 0xFFU);
    bit_writer writer = make_writer(buffer);
    writer.field(static_cast<uint8_t>(0xFU), 4U);
    writer.reserved(0U);
    EXPECT_EQ(writer.bit_size(), 4U);
    writer.reserved(1U);
    EXPECT_EQ(writer.bit_size(), 5U);
    writer.field(static_cast<uint8_t>(0x7U), 3U);
    writer.reserved(7U);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 15U);
    EXPECT_EQ(buffer[0U], 0xF7U); // 1111 0 111
    EXPECT_EQ(buffer[1U], 0x00U);
    EXPECT_EQ(buffer[2U], 0xFFU); // beyond the payload
}

TEST(RtcmBitWriter, ReservedSplitsRunsLongerThanSixtyFourBits)
{
    uint8_t buffer[17U];
    fill(buffer, 0xFFU);
    bit_writer writer = make_writer(buffer);
    writer.field(true);
    writer.reserved(64U);   // exactly one chunk
    EXPECT_EQ(writer.bit_size(), 65U);
    writer.reserved(65U);   // 64 + 1
    EXPECT_EQ(writer.bit_size(), 130U);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(buffer[0U], 0x80U);
    for (size_t i = 1U; i < 17U; ++i)
    {
        EXPECT_EQ(buffer[i], 0x00U) << "byte " << i;
    }

    uint8_t big[40U];
    fill(big, 0xFFU);
    bit_writer long_run = make_writer(big);
    long_run.reserved(300U); // 4 x 64 + 44
    ASSERT_TRUE(long_run.ok());
    EXPECT_EQ(long_run.bit_size(), 300U);
    EXPECT_EQ(long_run.size(), 38U);
    for (size_t i = 0U; i < 38U; ++i)
    {
        EXPECT_EQ(big[i], 0x00U) << "byte " << i;
    }
    EXPECT_EQ(big[38U], 0xFFU);
}

TEST(RtcmBitWriter, ReservedOverflowLatchesFull)
{
    uint8_t buffer[8U] = {};
    bit_writer writer = make_writer(buffer);
    writer.reserved(65U); // 64 fit, the extra bit does not
    EXPECT_EQ(writer.status(), castle::status::full);
    EXPECT_EQ(writer.bit_size(), 64U);

    bit_writer exact = make_writer(buffer);
    exact.reserved(64U);
    EXPECT_TRUE(exact.ok());
    EXPECT_EQ(exact.remaining_bits(), 0U);

    bit_writer many = make_writer(buffer);
    many.reserved(200U);
    EXPECT_EQ(many.status(), castle::status::full);
}

TEST(RtcmBitWriter, TextWritesEightBitCountThenCharacters)
{
    castle::container::string<16U> value;
    ASSERT_EQ(value.assign("ABC"), castle::status::ok);

    uint8_t buffer[8U] = {};
    bit_writer writer = make_writer(buffer);
    writer.text(value);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 32U);
    EXPECT_EQ(buffer[0U], 0x03U);
    EXPECT_EQ(buffer[1U], static_cast<uint8_t>('A'));
    EXPECT_EQ(buffer[2U], static_cast<uint8_t>('B'));
    EXPECT_EQ(buffer[3U], static_cast<uint8_t>('C'));
}

TEST(RtcmBitWriter, TextEmptyStringWritesOnlyZeroCount)
{
    castle::container::string<16U> value;
    uint8_t buffer[2U] = {0xEEU, 0xEEU};
    bit_writer writer = make_writer(buffer);
    writer.text(value);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 8U);
    EXPECT_EQ(buffer[0U], 0x00U);
    EXPECT_EQ(buffer[1U], 0xEEU);
}

TEST(RtcmBitWriter, TextAtUnalignedOffsetAndNonAsciiBytes)
{
    castle::container::string<4U> value;
    ASSERT_EQ(value.assign("\xE9Z"), castle::status::ok);

    uint8_t buffer[5U];
    fill(buffer, 0xFFU);
    bit_writer writer = make_writer(buffer);
    writer.field(static_cast<uint8_t>(0xAU), 4U);
    writer.text(value);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.bit_size(), 28U);
    // 1010 | 0000 0010 | 1110 1001 | 0101 1010 | pad
    EXPECT_EQ(buffer[0U], 0xA0U);
    EXPECT_EQ(buffer[1U], 0x2EU);
    EXPECT_EQ(buffer[2U], 0x95U);
    EXPECT_EQ(buffer[3U], 0xA0U);
    EXPECT_EQ(buffer[4U], 0xFFU);
}

TEST(RtcmBitWriter, TextOf255CharactersUsesFullCountRange)
{
    castle::container::string<255U> value;
    for (size_t i = 0U; i < 255U; ++i)
    {
        ASSERT_EQ(value.push_back(static_cast<char>('a' + (i % 26U))), castle::status::ok);
    }

    uint8_t buffer[256U] = {};
    bit_writer writer = make_writer(buffer);
    writer.text(value);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.size(), 256U);
    EXPECT_EQ(buffer[0U], 0xFFU);
    EXPECT_EQ(buffer[1U], static_cast<uint8_t>('a'));
    EXPECT_EQ(buffer[255U], static_cast<uint8_t>('a' + (254U % 26U)));
}

TEST(RtcmBitWriter, TextOverflowLatchesFullAndStops)
{
    castle::container::string<16U> value;
    ASSERT_EQ(value.assign("ABCD"), castle::status::ok);

    uint8_t buffer[4U] = {};
    bit_writer writer = make_writer(buffer);
    writer.text(value); // count + 4 chars = 5 bytes, only 4 available
    EXPECT_EQ(writer.status(), castle::status::full);
    EXPECT_EQ(writer.bit_size(), 32U);
    EXPECT_EQ(buffer[0U], 0x04U);
    EXPECT_EQ(buffer[3U], static_cast<uint8_t>('C'));

    // Exact fit.
    uint8_t exact[5U] = {};
    bit_writer fits = make_writer(exact);
    fits.text(value);
    EXPECT_TRUE(fits.ok());
    EXPECT_EQ(exact[4U], static_cast<uint8_t>('D'));
}

// -----------------------------------------------------------------------------
// encode_message
// -----------------------------------------------------------------------------

TEST(RtcmEncodeMessage, BuildsFrameWithExactBytes)
{
    uint8_t frame[32U];
    fill(frame, 0xFFU);
    castle::size_type written = 0xDEADU;
    ASSERT_EQ(encode_message(simple_message{}, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, 9U);

    // Payload AB CD A0: 12-bit 0xABC, 4-bit 0xD, 3-bit 0b101 and five zero pad bits
    // (the output buffer held 0xFF before). Trailer from an independent CRC-24Q.
    const uint8_t expected[9U] = {0xD3U, 0x00U, 0x03U, 0xABU, 0xCDU, 0xA0U, 0xF9U, 0xD6U, 0x9CU};
    for (size_t i = 0U; i < sizeof(expected); ++i)
    {
        EXPECT_EQ(frame[i], expected[i]) << "byte " << i;
    }
    EXPECT_EQ(frame[9U], 0xFFU); // beyond the frame is untouched
    EXPECT_EQ(castle::checksums::crc24q::calculate(frame, written), 0U);
}

TEST(RtcmEncodeMessage, ExactFitAndOneByteShort)
{
    uint8_t exact[9U] = {};
    castle::size_type written = 0xDEADU;
    ASSERT_EQ(encode_message(simple_message{}, exact, sizeof(exact), written), castle::status::ok);
    EXPECT_EQ(written, 9U);

    uint8_t short_by_one[8U];
    fill(short_by_one, 0xEEU);
    written = 0xDEADU;
    EXPECT_EQ(encode_message(simple_message{}, short_by_one, sizeof(short_by_one), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmEncodeMessage, NullOutputIsInvalidArgument)
{
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(encode_message(simple_message{}, nullptr, 64U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);

    written = 0xDEADU;
    EXPECT_EQ(encode_message(simple_message{}, nullptr, 0U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmEncodeMessage, CapacityBelowFrameOverheadIsFull)
{
    uint8_t frame[16U] = {};
    for (castle::size_type capacity = 0U; capacity < RTCM_FRAME_OVERHEAD; ++capacity)
    {
        castle::size_type written = 0xDEADU;
        EXPECT_EQ(encode_message(empty_message{}, frame, capacity, written), castle::status::full)
            << "capacity " << capacity;
        EXPECT_EQ(written, 0U);
    }
}

TEST(RtcmEncodeMessage, EmptyPayloadFitsInOverheadOnly)
{
    uint8_t frame[RTCM_FRAME_OVERHEAD];
    fill(frame, 0xEEU);
    castle::size_type written = 0xDEADU;
    ASSERT_EQ(encode_message(empty_message{}, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, RTCM_FRAME_OVERHEAD);

    const uint8_t expected[6U] = {0xD3U, 0x00U, 0x00U, 0x47U, 0xEAU, 0x4BU};
    for (size_t i = 0U; i < sizeof(expected); ++i)
    {
        EXPECT_EQ(frame[i], expected[i]) << "byte " << i;
    }

    // A message that needs payload bytes cannot use an overhead-sized buffer.
    written = 0xDEADU;
    EXPECT_EQ(encode_message(simple_message{}, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmEncodeMessage, WriterCapacityIsOutputMinusOverhead)
{
    uint8_t frame[64U] = {};
    probe_message probe;
    castle::size_type written = 0U;

    ASSERT_EQ(encode_message(probe, frame, 6U, written), castle::status::ok);
    EXPECT_EQ(probe.remaining, 0U);
    ASSERT_EQ(encode_message(probe, frame, 7U, written), castle::status::ok);
    EXPECT_EQ(probe.remaining, 8U);
    ASSERT_EQ(encode_message(probe, frame, 64U, written), castle::status::ok);
    EXPECT_EQ(probe.remaining, 58U * 8U);
}

TEST(RtcmEncodeMessage, PayloadIsCappedAtMaximumLength)
{
    static uint8_t frame[RTCM_MAX_FRAME_SIZE + 64U];
    castle::size_type written = 0U;
    probe_message probe;

    // Exactly the largest frame: payload capacity 1023, no capping needed.
    ASSERT_EQ(encode_message(probe, frame, RTCM_MAX_FRAME_SIZE, written), castle::status::ok);
    EXPECT_EQ(probe.remaining, RTCM_MAX_PAYLOAD_LEN * 8U);

    // Larger buffers do not raise the payload capacity.
    ASSERT_EQ(encode_message(probe, frame, RTCM_MAX_FRAME_SIZE + 1U, written), castle::status::ok);
    EXPECT_EQ(probe.remaining, RTCM_MAX_PAYLOAD_LEN * 8U);
    ASSERT_EQ(encode_message(probe, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(probe.remaining, RTCM_MAX_PAYLOAD_LEN * 8U);

    // A maximum-size payload frames correctly (header 03 FF, trailer from an independent CRC-24Q).
    fill(frame, 0xFFU);
    zero_bytes_message maximum = {RTCM_MAX_PAYLOAD_LEN};
    ASSERT_EQ(encode_message(maximum, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, RTCM_MAX_FRAME_SIZE);
    EXPECT_EQ(frame[0U], 0xD3U);
    EXPECT_EQ(frame[1U], 0x03U);
    EXPECT_EQ(frame[2U], 0xFFU);
    EXPECT_EQ(frame[3U], 0x00U);
    EXPECT_EQ(frame[written - 4U], 0x00U);
    EXPECT_EQ(frame[written - 3U], 0xFCU);
    EXPECT_EQ(frame[written - 2U], 0xD8U);
    EXPECT_EQ(frame[written - 1U], 0x01U);
    EXPECT_EQ(frame[written], 0xFFU);

    // One byte more than the limit overflows even though the buffer is large enough.
    zero_bytes_message too_long = {RTCM_MAX_PAYLOAD_LEN + 1U};
    written = 0xDEADU;
    EXPECT_EQ(encode_message(too_long, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmEncodeMessage, EncoderFailureIsPropagatedWithoutBytes)
{
    uint8_t frame[32U];
    fill(frame, 0xEEU);
    castle::size_type written = 0xDEADU;

    EXPECT_EQ(encode_message(failing_message{castle::status::invalid_argument}, frame, sizeof(frame), written),
              castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);

    written = 0xDEADU;
    EXPECT_EQ(encode_message(failing_message{castle::status::data_loss}, frame, sizeof(frame), written),
              castle::status::data_loss);
    EXPECT_EQ(written, 0U);

    written = 0xDEADU;
    EXPECT_EQ(encode_message(too_wide_message{}, frame, sizeof(frame), written), castle::status::out_of_range);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmEncodeMessage, EncoderOverflowBeyondPayloadCapacityIsFull)
{
    uint8_t frame[10U] = {}; // payload capacity 4 bytes
    castle::size_type written = 0xDEADU;
    zero_bytes_message fits = {4U};
    ASSERT_EQ(encode_message(fits, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, 10U);

    zero_bytes_message overflows = {5U};
    written = 0xDEADU;
    EXPECT_EQ(encode_message(overflows, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmEncodeMessage, Station1005MatchesGoldenFrame)
{
    messages::station_arp station;
    station.station_id = 1234U;
    station.itrf_year = 14U;
    station.gps = true;
    station.glonass = true;
    station.galileo = false;
    station.reference_station_indicator = false;
    station.single_receiver_oscillator = true;
    station.quarter_cycle_indicator = 2U;
    station.set_position_m(4027894.0060, 307045.6000, 4919474.9100);

    uint8_t frame[64U];
    fill(frame, 0xFFU);
    castle::size_type written = 0xDEADU;
    ASSERT_EQ(encode_message(station, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(golden_1005));
    for (size_t i = 0U; i < sizeof(golden_1005); ++i)
    {
        EXPECT_EQ(frame[i], golden_1005[i]) << "byte " << i;
    }

    // Exact fit and one byte short.
    uint8_t exact[sizeof(golden_1005)] = {};
    ASSERT_EQ(encode_message(station, exact, sizeof(exact), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(golden_1005));
    EXPECT_EQ(exact[sizeof(exact) - 1U], 0xF1U);

    uint8_t short_by_one[sizeof(golden_1005) - 1U] = {};
    written = 0xDEADU;
    EXPECT_EQ(encode_message(station, short_by_one, sizeof(short_by_one), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
