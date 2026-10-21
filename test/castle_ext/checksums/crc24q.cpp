#include <gtest/gtest.h>

#include "castle_ext/checksums/crc24q.hpp"
#include "castle_ext/checksums/crc_common.hpp"

#include <type_traits>

namespace
{

using castle::checksums::crc24q;
using castle::checksums::detail::basic_crc;

/** @brief "123456789", the standard CRC check message. */
const uint8_t check_message[9U] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

/** @brief Bytes 0x00..0xFF followed by "xyz": exercises every input byte value. */
struct long_message
{
    uint8_t bytes[259U];

    long_message()
    {
        for (size_t i = 0U; i < 256U; ++i)
        {
            bytes[i] = static_cast<uint8_t>(i);
        }
        bytes[256U] = 'x';
        bytes[257U] = 'y';
        bytes[258U] = 'z';
    }
};

// Custom Rocksoft parameter sets. Expected values come from the CRC RevEng
// catalogue check values (message "123456789") and, where no catalogue entry
// exists, from an independent bit-by-bit reference implementation.
// Reflected-input engines hold the register in reflected form, so their
// Initial template argument is the bit-reversed catalogue init value.

using crc24_openpgp = basic_crc<uint32_t, 0x864CFBU, 0xB704CEU, 0x000000U, false, false, 24U>;
using crc24_flexray_a = basic_crc<uint32_t, 0x5D6DCBU, 0xFEDCBAU, 0x000000U, false, false, 24U>;
using crc24_interlaken = basic_crc<uint32_t, 0x328B63U, 0xFFFFFFU, 0xFFFFFFU, false, false, 24U>;
using crc24_ble = basic_crc<uint32_t, 0x00065BU, 0xAAAAAAU, 0x000000U, true, true, 24U>;
using crc24_reflect_in_only = basic_crc<uint32_t, 0x864CFBU, 0x000000U, 0x000000U, true, false, 24U>;
using crc24_reflect_in_init_xor = basic_crc<uint32_t, 0x864CFBU, 0x6A2C48U, 0xA5A5A5U, true, false, 24U>;
using crc12_dect = basic_crc<uint16_t, 0x80FU, 0x000U, 0x000U, false, false, 12U>;
using crc12_umts = basic_crc<uint16_t, 0x80FU, 0x000U, 0x000U, false, true, 12U>;
using crc16_arc_in_u32 = basic_crc<uint32_t, 0x8005U, 0x0000U, 0x0000U, true, true, 16U>;
using crc8_smbus_in_u32 = basic_crc<uint32_t, 0x07U, 0x00U, 0x00U, false, false, 8U>;
using crc8_out_only = basic_crc<uint8_t, 0x31U, 0xFFU, 0x0FU, false, true>;

TEST(CastleExtCrc24q, CheckValueOfStandardMessage)
{
    EXPECT_EQ(crc24q::calculate(check_message, 9U), 0xCDE703U);

    crc24q crc;
    crc.update(check_message, 9U);
    EXPECT_EQ(crc.value(), 0xCDE703U);
    EXPECT_EQ(crc.checksum(), 0xCDE703U);
}

TEST(CastleExtCrc24q, ValueTypeIsUint32)
{
    static_assert(std::is_same<crc24q::value_type, uint32_t>::value, "24-bit CRC lives in a uint32_t");
    SUCCEED();
}

TEST(CastleExtCrc24q, LongMessageCoversEveryByteValue)
{
    const long_message message;
    EXPECT_EQ(crc24q::calculate(message.bytes, sizeof(message.bytes)), 0xA32F67U);
}

TEST(CastleExtCrc24q, InitialValueIsZeroAndEmptyMessageYieldsZero)
{
    crc24q crc;
    EXPECT_EQ(crc.value(), 0U);
    EXPECT_EQ(crc24q::calculate(check_message, 0U), 0U);
    EXPECT_EQ(crc24q::calculate(static_cast<const void*>(check_message), 0U), 0U);

    // Leading zero bytes keep a zero-initialised CRC at zero.
    const uint8_t zeros[4U] = {};
    EXPECT_EQ(crc24q::calculate(zeros, sizeof(zeros)), 0U);
}

TEST(CastleExtCrc24q, StreamingInChunksMatchesOneShot)
{
    const long_message message;
    const uint32_t expected = crc24q::calculate(message.bytes, sizeof(message.bytes));

    // Byte at a time.
    crc24q bytewise;
    for (size_t i = 0U; i < sizeof(message.bytes); ++i)
    {
        bytewise.update(&message.bytes[i], 1U);
    }
    EXPECT_EQ(bytewise.value(), expected);

    // Every possible two-chunk split.
    for (size_t split = 0U; split <= sizeof(message.bytes); ++split)
    {
        crc24q crc;
        crc.update(message.bytes, split);
        crc.update(message.bytes + split, sizeof(message.bytes) - split);
        ASSERT_EQ(crc.value(), expected) << "split at " << split;
    }

    // Check message in the 4 + 5 byte split.
    crc24q crc;
    crc.update(check_message, 4U);
    crc.update(check_message + 4U, 5U);
    EXPECT_EQ(crc.value(), 0xCDE703U);
}

TEST(CastleExtCrc24q, ValueDoesNotDisturbRunningState)
{
    crc24q crc;
    crc.update(check_message, 4U);
    const uint32_t partial = crc.value();
    EXPECT_EQ(crc.value(), partial);
    EXPECT_EQ(crc.checksum(), partial);
    EXPECT_NE(partial, 0xCDE703U);

    crc.update(check_message + 4U, 5U);
    EXPECT_EQ(crc.value(), 0xCDE703U);
}

TEST(CastleExtCrc24q, ResetRestoresInitialState)
{
    crc24q crc;
    crc.update(check_message, 9U);
    ASSERT_EQ(crc.value(), 0xCDE703U);

    crc.reset();
    EXPECT_EQ(crc.value(), 0U);

    crc.update(check_message, 9U);
    EXPECT_EQ(crc.value(), 0xCDE703U);
}

TEST(CastleExtCrc24q, VoidPointerOverloadsMatchBytePointerOverloads)
{
    const void* untyped = check_message;
    EXPECT_EQ(crc24q::calculate(untyped, 9U), 0xCDE703U);

    crc24q crc;
    crc.update(untyped, 3U);
    crc.update(static_cast<const void*>(check_message + 3U), 6U);
    EXPECT_EQ(crc.value(), 0xCDE703U);
}

TEST(CastleExtCrc24q, NullPointerIsIgnored)
{
    EXPECT_EQ(crc24q::calculate(static_cast<const uint8_t*>(nullptr), 5U), 0U);
    EXPECT_EQ(crc24q::calculate(static_cast<const void*>(nullptr), 5U), 0U);

    crc24q crc;
    crc.update(check_message, 4U);
    const uint32_t before = crc.value();
    crc.update(static_cast<const uint8_t*>(nullptr), 100U);
    EXPECT_EQ(crc.value(), before);
    crc.update(static_cast<const void*>(nullptr), 100U);
    EXPECT_EQ(crc.value(), before);

    crc.update(check_message + 4U, 5U);
    EXPECT_EQ(crc.value(), 0xCDE703U);
}

TEST(CastleExtCrc24q, ResultStaysWithin24Bits)
{
    uint8_t pattern[64U];
    for (size_t fill = 0U; fill < 256U; fill += 15U)
    {
        for (size_t length = 0U; length <= sizeof(pattern); length += 7U)
        {
            for (size_t i = 0U; i < sizeof(pattern); ++i)
            {
                pattern[i] = static_cast<uint8_t>(fill + i * 31U);
            }
            ASSERT_EQ(crc24q::calculate(pattern, length) & 0xFF000000U, 0U);
        }
    }

    const uint8_t ones[32U] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
                               0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
                               0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
                               0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    EXPECT_EQ(crc24q::calculate(ones, sizeof(ones)) & 0xFF000000U, 0U);
}

TEST(CastleExtCrc24q, SingleBitErrorsAreDetected)
{
    long_message message;
    const uint32_t reference = crc24q::calculate(message.bytes, sizeof(message.bytes));
    for (size_t byte = 0U; byte < sizeof(message.bytes); byte += 13U)
    {
        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            message.bytes[byte] = static_cast<uint8_t>(message.bytes[byte] ^ (1U << bit));
            ASSERT_NE(crc24q::calculate(message.bytes, sizeof(message.bytes)), reference);
            message.bytes[byte] = static_cast<uint8_t>(message.bytes[byte] ^ (1U << bit));
        }
    }
    EXPECT_EQ(crc24q::calculate(message.bytes, sizeof(message.bytes)), reference);
}

TEST(CastleExtCrc24q, AppendedCrcMakesRemainderZero)
{
    // CRC-24Q has no final XOR: appending the big-endian CRC yields a zero remainder.
    uint8_t buffer[12U];
    for (size_t i = 0U; i < 9U; ++i)
    {
        buffer[i] = check_message[i];
    }
    buffer[9U] = 0xCDU;
    buffer[10U] = 0xE7U;
    buffer[11U] = 0x03U;
    EXPECT_EQ(crc24q::calculate(buffer, sizeof(buffer)), 0U);
}

// --- Width handling of the shared engine ------------------------------------

TEST(CastleExtCrcCommon, NarrowWidthNonReflectedVariants)
{
    EXPECT_EQ(crc24_openpgp::calculate(check_message, 9U), 0x21CF02U);
    EXPECT_EQ(crc24_flexray_a::calculate(check_message, 9U), 0x7979BDU);
    EXPECT_EQ(crc24_interlaken::calculate(check_message, 9U), 0xB4F3E6U);
    EXPECT_EQ(crc12_dect::calculate(check_message, 9U), 0xF5BU);
    EXPECT_EQ(crc8_smbus_in_u32::calculate(check_message, 9U), 0xF4U);
}

TEST(CastleExtCrcCommon, NarrowWidthReflectedInput)
{
    // Reflected-input branch with a register wider than the CRC width.
    EXPECT_EQ(crc24_ble::calculate(check_message, 9U), 0xC25A56U);
    EXPECT_EQ(crc16_arc_in_u32::calculate(check_message, 9U), 0xBB3DU);
}

TEST(CastleExtCrcCommon, NarrowWidthMixedReflection)
{
    // Output reflected, input not.
    EXPECT_EQ(crc12_umts::calculate(check_message, 9U), 0xDAFU);
    // Input reflected, output not (reference implementation values).
    EXPECT_EQ(crc24_reflect_in_only::calculate(check_message, 9U), 0x2A3559U);
    EXPECT_EQ(crc24_reflect_in_init_xor::calculate(check_message, 9U), 0x007112U);
    // Full-width register with output-only reflection, final XOR and non-zero initial value.
    EXPECT_EQ(crc8_out_only::calculate(check_message, 9U), 0xE0U);
}

TEST(CastleExtCrcCommon, ResultNeverExceedsDeclaredWidth)
{
    const long_message message;
    EXPECT_EQ(crc24_ble::calculate(message.bytes, sizeof(message.bytes)) & ~0x00FFFFFFU, 0U);
    EXPECT_EQ(crc24_reflect_in_init_xor::calculate(message.bytes, sizeof(message.bytes)) & ~0x00FFFFFFU, 0U);
    EXPECT_EQ(crc24_interlaken::calculate(message.bytes, sizeof(message.bytes)) & ~0x00FFFFFFU, 0U);
    EXPECT_EQ(crc12_dect::calculate(message.bytes, sizeof(message.bytes)) & ~0x0FFFU, 0U);
    EXPECT_EQ(crc12_umts::calculate(message.bytes, sizeof(message.bytes)) & ~0x0FFFU, 0U);
    EXPECT_EQ(crc16_arc_in_u32::calculate(message.bytes, sizeof(message.bytes)) & ~0xFFFFU, 0U);
    EXPECT_EQ(crc8_smbus_in_u32::calculate(message.bytes, sizeof(message.bytes)) & ~0xFFU, 0U);

    // Long-message reference values.
    EXPECT_EQ(crc24_ble::calculate(message.bytes, sizeof(message.bytes)), 0x0D752DU);
    EXPECT_EQ(crc12_umts::calculate(message.bytes, sizeof(message.bytes)), 0x5B1U);
    EXPECT_EQ(crc12_dect::calculate(message.bytes, sizeof(message.bytes)), 0x8DAU);
    EXPECT_EQ(crc24_openpgp::calculate(message.bytes, sizeof(message.bytes)), 0xD9B1EAU);
    EXPECT_EQ(crc24_flexray_a::calculate(message.bytes, sizeof(message.bytes)), 0xCF6B6FU);
    EXPECT_EQ(crc24_interlaken::calculate(message.bytes, sizeof(message.bytes)), 0x83B01BU);
    EXPECT_EQ(crc8_smbus_in_u32::calculate(message.bytes, sizeof(message.bytes)), 0x41U);
    EXPECT_EQ(crc16_arc_in_u32::calculate(message.bytes, sizeof(message.bytes)), 0xF3A0U);
    EXPECT_EQ(crc24_reflect_in_only::calculate(message.bytes, sizeof(message.bytes)), 0x676C45U);
    EXPECT_EQ(crc8_out_only::calculate(message.bytes, sizeof(message.bytes)), 0xDBU);
}

TEST(CastleExtCrcCommon, NarrowWidthStreamingResetAndNullHandling)
{
    crc24_ble crc;
    EXPECT_EQ(crc.value(), 0xAAAAAAU); // reflected initial register, no final XOR

    crc.update(check_message, 5U);
    crc.update(static_cast<const uint8_t*>(nullptr), 3U);
    crc.update(static_cast<const void*>(check_message + 5U), 4U);
    EXPECT_EQ(crc.value(), 0xC25A56U);
    EXPECT_EQ(crc.checksum(), 0xC25A56U);

    crc.reset();
    EXPECT_EQ(crc.value(), 0xAAAAAAU);
    EXPECT_EQ(crc24_ble::calculate(static_cast<const void*>(check_message), 9U), 0xC25A56U);
    EXPECT_EQ(crc24_ble::calculate(static_cast<const uint8_t*>(nullptr), 9U), 0xAAAAAAU);

    crc12_umts umts;
    umts.update(check_message, 9U);
    EXPECT_EQ(umts.value(), 0xDAFU);
    umts.reset();
    EXPECT_EQ(umts.value(), 0U);
}

TEST(CastleExtCrcCommon, DefaultWidthIsFullRegisterWidth)
{
    // Omitting Width keeps the historic CRC-8/16/32/64 behaviour.
    using crc16_default = basic_crc<uint16_t, 0x1021U, 0xFFFFU, 0x0000U, false, false>;
    using crc16_explicit = basic_crc<uint16_t, 0x1021U, 0xFFFFU, 0x0000U, false, false, 16U>;
    EXPECT_EQ(crc16_default::calculate(check_message, 9U), 0x29B1U);
    EXPECT_EQ(crc16_explicit::calculate(check_message, 9U), 0x29B1U);

    using crc32_default = basic_crc<uint32_t, 0x04C11DB7U, 0xFFFFFFFFU, 0xFFFFFFFFU, true, true>;
    EXPECT_EQ(crc32_default::calculate(check_message, 9U), 0xCBF43926U);

    using crc64_default = basic_crc<uint64_t, 0x42F0E1EBA9EA3693ULL, 0ULL, 0ULL, false, false>;
    EXPECT_EQ(crc64_default::calculate(check_message, 9U), 0x6C40DF5F0B497347ULL);
}

} // namespace
