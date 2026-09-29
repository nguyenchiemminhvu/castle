#include <gtest/gtest.h>

#include "castle_ext/protocols/lin/lin.hpp"

namespace
{
using namespace castle::protocols::lin;

TEST(LinUmbrella, ExposesFrameChecksumScheduleAndNodes)
{
    const uint8_t payload[1U] = {0xA5U};
    frame response{};
    ASSERT_EQ(make_frame(0x22U, payload, 1U, checksum_type::enhanced, response), castle::status::ok);
    EXPECT_TRUE(validate_checksum(response));

    schedule_table<2U> schedule;
    EXPECT_EQ(schedule.add(response.identifier, response_owner::slave, 4U), castle::status::ok);
    schedule_entry entry{};
    EXPECT_EQ(schedule.next(entry), castle::status::ok);
    EXPECT_EQ(entry.identifier, response.identifier);

    master_node<> master;
    uint8_t seen_id = 0xFFU;
    master.configure_header_transmitter([&seen_id](const header& request) {
        seen_id = request.identifier;
        return castle::status::ok;
    });
    EXPECT_EQ(master.transmit_slot(entry), castle::status::ok);
    EXPECT_EQ(seen_id, response.identifier);
}
} // namespace
