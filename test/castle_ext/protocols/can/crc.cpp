#include <gtest/gtest.h>

#include "castle_ext/protocols/can/crc.hpp"

namespace
{
using namespace castle::protocols::can;

TEST(CanCrc, ClassicalCrc15ReferenceValue)
{
    const uint8_t payload[2U] = {0x11U, 0x22U};
    frame value{};
    ASSERT_EQ(make_data_frame(0x123U, identifier_format::standard,
        frame_format::classical, payload, 2U, value), castle::status::ok);
    crc_result result{};
    EXPECT_EQ(calculate_crc(value, result), castle::status::ok);
    EXPECT_EQ(result.width, CAN_CLASSICAL_CRC_WIDTH);
    EXPECT_EQ(result.value, 0x04B7U);
}

TEST(CanCrc, CanFdCrc17KnownVector)
{
    // ISO CAN FD vector: standard ID 0x5AA, BRS=1, ESI=0, eight 0xFF bytes.
    const uint8_t payload[8U] = {0xFFU, 0xFFU, 0xFFU, 0xFFU,
                                 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    frame value{};
    ASSERT_EQ(make_data_frame(0x5AAU, identifier_format::standard,
        frame_format::fd, payload, 8U, value, true, false), castle::status::ok);
    crc_result result{};
    EXPECT_EQ(calculate_crc(value, result), castle::status::ok);
    EXPECT_EQ(result.width, CAN_FD_CRC17_WIDTH);
    EXPECT_EQ(result.value, 0x006BDU);
}

TEST(CanCrc, CanFdCrc21ForLongFrames)
{
    uint8_t payload[64U] = {};
    frame value{};
    ASSERT_EQ(make_data_frame(0x1ABCDEU, identifier_format::extended,
        frame_format::fd, payload, 20U, value, true, true), castle::status::ok);
    crc_result result{};
    EXPECT_EQ(calculate_crc(value, result), castle::status::ok);
    EXPECT_EQ(result.width, CAN_FD_CRC21_WIDTH);
    EXPECT_LE(result.value, 0x1FFFFFU);

    payload[19U] = 0xFFU;
    frame changed{};
    ASSERT_EQ(make_data_frame(0x1ABCDEU, identifier_format::extended,
        frame_format::fd, payload, 20U, changed, true, true), castle::status::ok);
    crc_result changed_result{};
    ASSERT_EQ(calculate_crc(changed, changed_result), castle::status::ok);
    EXPECT_NE(changed_result.value, result.value);
}

TEST(CanCrc, RemoteFrameAndEmptyDataAreSupported)
{
    frame value{};
    ASSERT_EQ(make_remote_frame(0x321U, identifier_format::standard, 8U, value), castle::status::ok);
    crc_result result{};
    EXPECT_EQ(calculate_crc(value, result), castle::status::ok);
    EXPECT_EQ(result.width, CAN_CLASSICAL_CRC_WIDTH);

    ASSERT_EQ(make_data_frame(0x321U, identifier_format::standard,
        frame_format::classical, nullptr, 0U, value), castle::status::ok);
    EXPECT_EQ(calculate_crc(value, result), castle::status::ok);
}

TEST(CanCrc, InvalidSignalAndMalformedFramesAreRejectedAndOutputCleared)
{
    frame signal{};
    ASSERT_EQ(make_error_frame(0U, identifier_format::standard, signal), castle::status::ok);
    crc_result result{0x123U, 9U};
    EXPECT_EQ(calculate_crc(signal, result), castle::status::invalid_argument);
    EXPECT_EQ(result.value, 0U);
    EXPECT_EQ(result.width, 0U);

    signal.kind = frame_kind::data;
    signal.data_length = 9U;
    EXPECT_EQ(calculate_crc(signal, result), castle::status::invalid_argument);
}

} // namespace
