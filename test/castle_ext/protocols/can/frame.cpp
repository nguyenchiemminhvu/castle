#include <gtest/gtest.h>

#include "castle_ext/protocols/can/frame.hpp"

namespace
{
using namespace castle::protocols::can;

TEST(CanFrame, IdentifierBoundariesAndFormats)
{
    frame value{};
    EXPECT_EQ(make_data_frame(CAN_STANDARD_IDENTIFIER_MAX, identifier_format::standard,
        frame_format::classical, nullptr, 0U, value), castle::status::ok);
    EXPECT_TRUE(value.valid());
    EXPECT_EQ(value.identifier, CAN_STANDARD_IDENTIFIER_MAX);

    EXPECT_EQ(make_data_frame(CAN_EXTENDED_IDENTIFIER_MAX, identifier_format::extended,
        frame_format::classical, nullptr, 0U, value), castle::status::ok);
    EXPECT_TRUE(value.valid());
    EXPECT_EQ(value.identifier, CAN_EXTENDED_IDENTIFIER_MAX);

    EXPECT_EQ(make_data_frame(CAN_STANDARD_IDENTIFIER_MAX + 1U, identifier_format::standard,
        frame_format::classical, nullptr, 0U, value), castle::status::invalid_argument);
    EXPECT_EQ(make_data_frame(CAN_EXTENDED_IDENTIFIER_MAX + 1U, identifier_format::extended,
        frame_format::classical, nullptr, 0U, value), castle::status::invalid_argument);
    EXPECT_EQ(make_data_frame(0U, static_cast<identifier_format>(9U),
        frame_format::classical, nullptr, 0U, value), castle::status::invalid_argument);
}

TEST(CanFrame, ClassicalDataAndRemoteLengths)
{
    uint8_t bytes[8U] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U};
    frame value{};
    for (castle::size_type length = 0U; length <= CAN_CLASSICAL_MAX_DATA_LENGTH; ++length)
    {
        EXPECT_EQ(make_data_frame(0x100U, identifier_format::standard,
            frame_format::classical, bytes, length, value), castle::status::ok);
        EXPECT_TRUE(value.valid());
        EXPECT_EQ(value.data_length_code(), length);
    }
    EXPECT_EQ(make_data_frame(0x100U, identifier_format::standard,
        frame_format::classical, bytes, 9U, value), castle::status::invalid_argument);

    EXPECT_EQ(make_remote_frame(0x123U, identifier_format::standard, 8U, value), castle::status::ok);
    EXPECT_TRUE(value.valid());
    EXPECT_FALSE(value.has_data());
    EXPECT_EQ(value.kind, frame_kind::remote);
    EXPECT_EQ(value.data_length_code(), 8U);
    EXPECT_EQ(make_remote_frame(0x123U, identifier_format::standard, 9U, value),
        castle::status::invalid_argument);
    EXPECT_EQ(make_remote_frame(0x123U, identifier_format::extended, 0U, value), castle::status::ok);
}

TEST(CanFrame, ArrayViewFactoryCopiesPayload)
{
    const uint8_t payload[3U] = {0x12U, 0x34U, 0x56U};
    const castle::container::array_view<const uint8_t> view(payload, 3U);
    frame value{};
    EXPECT_EQ(make_data_frame(0x124U, identifier_format::standard,
        frame_format::classical, view, value), castle::status::ok);
    EXPECT_EQ(value.data_length, 3U);
    EXPECT_EQ(value.data[0U], 0x12U);
    EXPECT_EQ(value.data[2U], 0x56U);
}

TEST(CanFrame, FdDlcMappingAndValidation)
{
    const castle::size_type legal_lengths[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U,
                                                12U, 16U, 20U, 24U, 32U, 48U, 64U};
    const uint8_t expected_dlc[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U,
                                    9U, 10U, 11U, 12U, 13U, 14U, 15U};
    uint8_t bytes[64U] = {};
    frame value{};
    for (castle::size_type i = 0U; i < sizeof(legal_lengths) / sizeof(legal_lengths[0U]); ++i)
    {
        EXPECT_TRUE(is_valid_fd_data_length(legal_lengths[i]));
        EXPECT_EQ(fd_data_length_code(legal_lengths[i]), expected_dlc[i]);
        EXPECT_EQ(make_data_frame(0x1ABCDEU, identifier_format::extended,
            frame_format::fd, bytes, legal_lengths[i], value, true, true), castle::status::ok);
        EXPECT_TRUE(value.valid());
        EXPECT_TRUE(value.is_fd());
        EXPECT_EQ(value.data_length_code(), expected_dlc[i]);
    }
    const castle::size_type illegal_lengths[] = {9U, 10U, 11U, 13U, 15U, 17U, 23U, 31U, 63U, 65U};
    for (castle::size_type i = 0U; i < sizeof(illegal_lengths) / sizeof(illegal_lengths[0U]); ++i)
    {
        EXPECT_FALSE(is_valid_fd_data_length(illegal_lengths[i]));
        EXPECT_EQ(fd_data_length_code(illegal_lengths[i]), 0xFFU);
    }
    EXPECT_EQ(make_data_frame(0x123U, identifier_format::standard, frame_format::fd,
        bytes, 9U, value), castle::status::invalid_argument);
}

TEST(CanFrame, FactoryRejectsNullOrOverCapacityAndLeavesOutputUnchanged)
{
    frame value{};
    value.identifier = 0x42U;
    EXPECT_EQ(make_data_frame(0x100U, identifier_format::standard,
        frame_format::classical, nullptr, 1U, value), castle::status::invalid_argument);
    EXPECT_EQ(value.identifier, 0x42U);

    uint8_t data[65U] = {};
    EXPECT_EQ(make_data_frame(0x100U, identifier_format::standard,
        frame_format::fd, data, 65U, value), castle::status::invalid_argument);
    EXPECT_EQ(make_data_frame(0U, identifier_format::standard, frame_format::classical,
        nullptr, 0U, value, true, false), castle::status::invalid_argument);
    EXPECT_EQ(make_data_frame(0U, identifier_format::standard, frame_format::classical,
        nullptr, 0U, value, false, true), castle::status::invalid_argument);
}

TEST(CanFrame, ErrorOverloadAndMalformedEnumValidation)
{
    frame value{};
    EXPECT_EQ(make_error_frame(0U, identifier_format::standard, value), castle::status::ok);
    EXPECT_TRUE(value.valid());
    EXPECT_EQ(value.kind, frame_kind::error);
    EXPECT_EQ(value.data_length_code(), 0xFFU);
    EXPECT_EQ(make_overload_frame(0U, identifier_format::standard, value), castle::status::ok);
    EXPECT_TRUE(value.valid());
    EXPECT_EQ(value.kind, frame_kind::overload);
    EXPECT_EQ(value.data_length_code(), 0xFFU);

    value.kind = static_cast<frame_kind>(10U);
    EXPECT_FALSE(value.valid());
    value.kind = frame_kind::data;
    value.format = static_cast<frame_format>(10U);
    EXPECT_FALSE(value.valid());
    value.format = frame_format::classical;
    value.data_length = 9U;
    EXPECT_FALSE(value.valid());
    value.data_length = 0U;
    value.identifier = CAN_EXTENDED_IDENTIFIER_MAX + 1U;
    EXPECT_FALSE(value.valid());
}

TEST(CanFrame, DisallowedFdKindsAndFlagsAreRejected)
{
    frame value{};
    value.format = frame_format::fd;
    value.kind = frame_kind::remote;
    EXPECT_FALSE(value.valid());
    value.kind = frame_kind::error;
    EXPECT_FALSE(value.valid());
    value.kind = frame_kind::overload;
    EXPECT_FALSE(value.valid());

    value.format = frame_format::classical;
    value.kind = frame_kind::error;
    value.bit_rate_switch = true;
    EXPECT_FALSE(value.valid());
    value.bit_rate_switch = false;
    value.error_state_indicator = true;
    EXPECT_FALSE(value.valid());
}

} // namespace
