#include <gtest/gtest.h>

#include "castle_ext/protocols/lin/frame.hpp"

namespace
{
using namespace castle::protocols::lin;

TEST(LinFrame, IdentifierAndPidParity)
{
    EXPECT_TRUE(is_valid_identifier(0U));
    EXPECT_TRUE(is_valid_identifier(61U));
    EXPECT_FALSE(is_valid_identifier(62U));
    EXPECT_FALSE(is_valid_identifier(63U));
    EXPECT_EQ(calculate_protected_identifier(0x12U), 0x92U);
    EXPECT_EQ(calculate_protected_identifier(62U), LIN_INVALID_PROTECTED_IDENTIFIER);
    EXPECT_EQ(calculate_protected_identifier(256U), LIN_INVALID_PROTECTED_IDENTIFIER);
    EXPECT_TRUE(is_valid_protected_identifier(0x92U));
    EXPECT_FALSE(is_valid_protected_identifier(0x12U));
    EXPECT_FALSE(is_valid_protected_identifier(0xFEU));
}

TEST(LinFrame, HeaderFactoriesPreserveOutputOnFailure)
{
    header request{};
    EXPECT_EQ(make_header(0x12U, request), castle::status::ok);
    EXPECT_TRUE(request.valid());
    EXPECT_EQ(request.identifier, 0x12U);
    EXPECT_EQ(request.protected_identifier, 0x92U);

    header decoded{};
    EXPECT_EQ(decode_header(request.protected_identifier, decoded), castle::status::ok);
    EXPECT_EQ(decoded.identifier, request.identifier);
    EXPECT_EQ(decoded.protected_identifier, request.protected_identifier);

    const uint8_t saved_id = decoded.identifier;
    const uint8_t saved_pid = decoded.protected_identifier;
    EXPECT_EQ(make_header(62U, decoded), castle::status::invalid_argument);
    EXPECT_EQ(make_header(256U, decoded), castle::status::invalid_argument);
    EXPECT_EQ(decoded.identifier, saved_id);
    EXPECT_EQ(decoded.protected_identifier, saved_pid);
    EXPECT_EQ(decode_header(0x12U, decoded), castle::status::invalid_argument);
    EXPECT_EQ(decoded.protected_identifier, saved_pid);

    request.protected_identifier ^= 0x40U;
    EXPECT_FALSE(request.valid());
}

TEST(LinFrame, FrameShapeValidation)
{
    frame value{};
    EXPECT_TRUE(value.valid());
    value.identifier = 62U;
    EXPECT_FALSE(value.valid());
    value.identifier = 0U;
    value.protected_identifier = 0x00U;
    EXPECT_FALSE(value.valid());
    value.protected_identifier = calculate_protected_identifier(0U);
    value.data_length = 0U;
    EXPECT_FALSE(value.valid());
    value.data_length = 9U;
    EXPECT_FALSE(value.valid());
    value.data_length = 1U;
    value.checksum_mode = static_cast<checksum_type>(9U);
    EXPECT_FALSE(value.valid());

    value.identifier = LIN_DIAGNOSTIC_REQUEST_IDENTIFIER;
    value.protected_identifier = calculate_protected_identifier(value.identifier);
    value.data_length = 7U;
    value.checksum_mode = checksum_type::classic;
    EXPECT_FALSE(value.valid());
    value.data_length = 8U;
    value.checksum_mode = checksum_type::enhanced;
    EXPECT_FALSE(value.valid());
    value.checksum_mode = checksum_type::classic;
    EXPECT_TRUE(value.valid());
}
} // namespace
