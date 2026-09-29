#include <gtest/gtest.h>

#include "castle/container/array_view.hpp"
#include "castle_ext/protocols/lin/checksum.hpp"

namespace
{
using namespace castle::protocols::lin;

TEST(LinChecksum, ClassicAndEnhancedExamples)
{
    const uint8_t data[1U] = {0x4AU};
    uint8_t checksum = 0U;
    EXPECT_EQ(calculate_checksum(0x12U, data, 1U, checksum_type::classic, checksum), castle::status::ok);
    EXPECT_EQ(checksum, 0xB5U);
    EXPECT_EQ(calculate_checksum(0x12U, data, 1U, checksum_type::enhanced, checksum), castle::status::ok);
    EXPECT_EQ(checksum, 0x23U);
    const uint8_t carry_data[2U] = {0xF0U, 0xFFU};
    EXPECT_EQ(calculate_checksum(0x12U, carry_data, 2U, checksum_type::classic, checksum), castle::status::ok);
    EXPECT_EQ(checksum, 0x0FU);
    EXPECT_EQ(calculate_checksum(0x12U, carry_data, 2U, checksum_type::enhanced, checksum), castle::status::ok);
    EXPECT_EQ(checksum, 0x7CU);
    EXPECT_EQ(effective_checksum_type(0x3CU, checksum_type::enhanced), checksum_type::classic);
    EXPECT_EQ(effective_checksum_type(0x3DU, checksum_type::enhanced), checksum_type::classic);
    EXPECT_EQ(effective_checksum_type(0x12U, checksum_type::classic), checksum_type::classic);
    EXPECT_EQ(effective_checksum_type(0x12U, checksum_type::enhanced), checksum_type::enhanced);
}

TEST(LinChecksum, InvalidArgumentsDoNotOverwriteOutput)
{
    const uint8_t data[1U] = {0xAAU};
    uint8_t checksum = 0x5AU;
    EXPECT_EQ(calculate_checksum(62U, data, 1U, checksum_type::classic, checksum), castle::status::invalid_argument);
    EXPECT_EQ(calculate_checksum(256U, data, 1U, checksum_type::classic, checksum), castle::status::invalid_argument);
    EXPECT_EQ(checksum, 0x5AU);
    EXPECT_EQ(calculate_checksum(1U, nullptr, 1U, checksum_type::classic, checksum), castle::status::invalid_argument);
    EXPECT_EQ(calculate_checksum(1U, data, 0U, checksum_type::classic, checksum), castle::status::invalid_argument);
    EXPECT_EQ(calculate_checksum(1U, data, 9U, checksum_type::classic, checksum), castle::status::invalid_argument);
    EXPECT_EQ(calculate_checksum(1U, data, 1U, static_cast<checksum_type>(9U), checksum), castle::status::invalid_argument);
    EXPECT_EQ(calculate_checksum(0x3CU, data, 1U, checksum_type::classic, checksum), castle::status::invalid_argument);
    EXPECT_EQ(calculate_checksum(1U, castle::container::array_view<const uint8_t>(), checksum_type::classic, checksum), castle::status::invalid_argument);
}

TEST(LinChecksum, FrameFactoryValidatesAndPreservesOutput)
{
    const uint8_t data[2U] = {0x12U, 0x34U};
    frame value{};
    EXPECT_EQ(make_frame(0x12U, data, 2U, checksum_type::enhanced, value), castle::status::ok);
    EXPECT_TRUE(value.valid());
    EXPECT_TRUE(validate_checksum(value));
    uint8_t calculated = 0U;
    EXPECT_EQ(calculate_checksum(value, calculated), castle::status::ok);
    EXPECT_EQ(calculated, value.checksum);
    const castle::container::array_view<const uint8_t> view(data, 2U);
    frame viewed{};
    EXPECT_EQ(make_frame(0x12U, view, checksum_type::enhanced, viewed), castle::status::ok);
    EXPECT_TRUE(validate_checksum(viewed));
    EXPECT_EQ(calculate_checksum(0x12U, view, checksum_type::enhanced, calculated), castle::status::ok);
    EXPECT_EQ(calculated, viewed.checksum);

    const uint8_t previous_id = value.identifier;
    EXPECT_EQ(make_frame(62U, data, 2U, checksum_type::classic, value), castle::status::invalid_argument);
    EXPECT_EQ(make_frame(256U, data, 2U, checksum_type::classic, value), castle::status::invalid_argument);
    EXPECT_EQ(make_frame(0x12U, nullptr, 2U, checksum_type::classic, value), castle::status::invalid_argument);
    EXPECT_EQ(make_frame(0x12U, data, 0U, checksum_type::classic, value), castle::status::invalid_argument);
    EXPECT_EQ(make_frame(0x12U, data, 9U, checksum_type::classic, value), castle::status::invalid_argument);
    EXPECT_EQ(make_frame(0x12U, data, 1U, static_cast<checksum_type>(3U), value), castle::status::invalid_argument);
    EXPECT_EQ(make_frame(0x3CU, data, 2U, checksum_type::classic, value), castle::status::invalid_argument);
    EXPECT_EQ(value.identifier, previous_id);

    value.data[0U] ^= 0x01U;
    EXPECT_FALSE(validate_checksum(value));
    value.data_length = 9U;
    EXPECT_EQ(calculate_checksum(value, calculated), castle::status::invalid_argument);
    EXPECT_FALSE(validate_checksum(value));
}

TEST(LinChecksum, DiagnosticFramesForceClassicAndRequireEightBytes)
{
    const uint8_t data[LIN_MAX_DATA_LENGTH] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U};
    frame request{};
    EXPECT_EQ(make_frame(0x3CU, data, LIN_MAX_DATA_LENGTH, checksum_type::enhanced, request), castle::status::ok);
    EXPECT_EQ(request.checksum_mode, checksum_type::classic);
    EXPECT_TRUE(validate_checksum(request));
    EXPECT_EQ(make_frame(0x3DU, data, LIN_MAX_DATA_LENGTH, checksum_type::classic, request), castle::status::ok);
    EXPECT_TRUE(validate_checksum(request));
}
} // namespace
