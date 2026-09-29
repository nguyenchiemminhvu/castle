#include <gtest/gtest.h>

#include "castle_ext/protocols/can/can.hpp"

TEST(CanUmbrella, ExposesFrameAndChecksumApi)
{
    using namespace castle::protocols::can;
    frame value{};
    ASSERT_EQ(make_data_frame(0x45U, identifier_format::standard,
        frame_format::classical, nullptr, 0U, value), castle::status::ok);
    crc_result result{};
    EXPECT_EQ(calculate_crc(value, result), castle::status::ok);
    EXPECT_EQ(result.width, CAN_CLASSICAL_CRC_WIDTH);
    EXPECT_TRUE(value.valid());
}
