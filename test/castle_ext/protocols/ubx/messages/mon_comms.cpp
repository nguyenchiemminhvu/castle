#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_comms.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, uint16_t length, uint8_t msg_class = UBX_CLASS_MON,
                     uint8_t msg_id = UBX_ID_MON_COMMS)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

mon_comms_port distinctive_port(uint16_t seed)
{
    mon_comms_port port{};
    port.port_id = static_cast<uint16_t>(0x0100U + seed);
    port.tx_pending = static_cast<uint16_t>(0x0200U + seed);
    port.tx_bytes = 0x03000000U + seed;
    port.tx_usage = static_cast<uint8_t>(10U + seed);
    port.tx_peak_usage = static_cast<uint8_t>(20U + seed);
    port.rx_pending = static_cast<uint16_t>(0x0400U + seed);
    port.rx_bytes = 0x05000000U + seed;
    port.rx_usage = static_cast<uint8_t>(30U + seed);
    port.rx_peak_usage = static_cast<uint8_t>(40U + seed);
    port.overrun_errs = static_cast<uint16_t>(0x0600U + seed);
    port.msgs[0U] = static_cast<uint16_t>(0x0700U + seed);
    port.msgs[1U] = static_cast<uint16_t>(0x0800U + seed);
    port.msgs[2U] = static_cast<uint16_t>(0x0900U + seed);
    port.msgs[3U] = static_cast<uint16_t>(0x0A00U + seed);
    port.skipped = 0x0B000000U + seed;
    return port;
}

void expect_port_eq(CASTLE_CONST mon_comms_port& actual, CASTLE_CONST mon_comms_port& expected)
{
    EXPECT_EQ(actual.port_id, expected.port_id);
    EXPECT_EQ(actual.tx_pending, expected.tx_pending);
    EXPECT_EQ(actual.tx_bytes, expected.tx_bytes);
    EXPECT_EQ(actual.tx_usage, expected.tx_usage);
    EXPECT_EQ(actual.tx_peak_usage, expected.tx_peak_usage);
    EXPECT_EQ(actual.rx_pending, expected.rx_pending);
    EXPECT_EQ(actual.rx_bytes, expected.rx_bytes);
    EXPECT_EQ(actual.rx_usage, expected.rx_usage);
    EXPECT_EQ(actual.rx_peak_usage, expected.rx_peak_usage);
    EXPECT_EQ(actual.overrun_errs, expected.overrun_errs);
    for (castle::size_type slot = 0U; slot < MON_COMMS_PROTOCOL_SLOTS; ++slot)
    {
        EXPECT_EQ(actual.msgs[slot], expected.msgs[slot]) << "msgs slot " << slot;
    }
    EXPECT_EQ(actual.skipped, expected.skipped);
}

TEST(UbxMessageMonComms, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_comms<>::msg_class, 0x0AU);
    EXPECT_EQ(mon_comms<>::msg_id, 0x36U);
    EXPECT_EQ(mon_comms<>::header_length, 8U);
    EXPECT_EQ(mon_comms<>::block_length, 40U);
    EXPECT_EQ(MON_COMMS_PROTOCOL_SLOTS, 4U);
    EXPECT_EQ(static_cast<uint8_t>(mon_comms_protocol::ubx), 0U);
    EXPECT_EQ(static_cast<uint8_t>(mon_comms_protocol::nmea), 1U);
    EXPECT_EQ(static_cast<uint8_t>(mon_comms_protocol::rtcm2), 2U);
    EXPECT_EQ(static_cast<uint8_t>(mon_comms_protocol::rtcm3), 5U);
    EXPECT_EQ(static_cast<uint8_t>(mon_comms_protocol::spartn), 6U);
    EXPECT_EQ(static_cast<uint8_t>(mon_comms_protocol::none), 0xFFU);

    mon_comms<> fresh{};
    for (castle::size_type slot = 0U; slot < MON_COMMS_PROTOCOL_SLOTS; ++slot)
    {
        EXPECT_EQ(fresh.prot_ids[slot], mon_comms_protocol::none);
    }
}

TEST(UbxMessageMonComms, RoundTripThroughFrame)
{
    mon_comms<> message{};
    message.version = 0x00U;
    message.n_ports = 42U; // ignored by encode
    message.tx_errors = 0x03U;
    message.prot_ids[0U] = mon_comms_protocol::ubx;
    message.prot_ids[1U] = mon_comms_protocol::nmea;
    message.prot_ids[2U] = mon_comms_protocol::rtcm3;
    message.prot_ids[3U] = mon_comms_protocol::spartn;
    ASSERT_EQ(message.ports.push_back(distinctive_port(1U)), castle::status::ok);
    ASSERT_EQ(message.ports.push_back(distinctive_port(2U)), castle::status::ok);

    uint8_t frame[128U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(8U + (2U * 40U)));
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[1U], 2U);
    EXPECT_EQ(payload[3U], 0U);
    for (castle::size_type offset = 36U; offset < 44U; ++offset)
    {
        EXPECT_EQ(payload[offset], 0U) << "reserved1 offset " << offset;
    }

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    ASSERT_TRUE(mon_comms<>::matches(raw));

    mon_comms<> decoded{};
    ASSERT_TRUE(mon_comms<>::decode(raw, decoded));
    EXPECT_EQ(decoded.version, 0x00U);
    EXPECT_EQ(decoded.n_ports, 2U);
    EXPECT_EQ(decoded.tx_errors, 0x03U);
    EXPECT_EQ(decoded.prot_ids[0U], mon_comms_protocol::ubx);
    EXPECT_EQ(decoded.prot_ids[1U], mon_comms_protocol::nmea);
    EXPECT_EQ(decoded.prot_ids[2U], mon_comms_protocol::rtcm3);
    EXPECT_EQ(decoded.prot_ids[3U], mon_comms_protocol::spartn);
    ASSERT_EQ(decoded.ports.size(), 2U);
    expect_port_eq(decoded.ports[0U], distinctive_port(1U));
    expect_port_eq(decoded.ports[1U], distinctive_port(2U));
}

TEST(UbxMessageMonComms, HandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[8U + 40U] = {};
    payload[0U] = 0x00U;
    payload[1U] = 1U;
    payload[2U] = 0x02U; // alloc
    payload[3U] = 0xEEU; // reserved0
    payload[4U] = 0x01U; // NMEA
    payload[5U] = 0x00U; // UBX
    payload[6U] = 0x02U; // RTCM2
    payload[7U] = 0xFFU; // none
    castle::write_le16(&payload[8U], 0x0101U);
    castle::write_le16(&payload[10U], 0x1234U);
    castle::write_le32(&payload[12U], 0x89ABCDEFU);
    payload[16U] = 55U;
    payload[17U] = 66U;
    castle::write_le16(&payload[18U], 0x4321U);
    castle::write_le32(&payload[20U], 0x76543210U);
    payload[24U] = 77U;
    payload[25U] = 88U;
    castle::write_le16(&payload[26U], 0x0F0FU);
    castle::write_le16(&payload[28U], 0x1001U);
    castle::write_le16(&payload[30U], 0x2002U);
    castle::write_le16(&payload[32U], 0x3003U);
    castle::write_le16(&payload[34U], 0x4004U);
    for (castle::size_type offset = 36U; offset < 44U; ++offset)
    {
        payload[offset] = 0xEEU; // reserved1
    }
    castle::write_le32(&payload[44U], 0xFEEDF00DU);

    mon_comms<> decoded{};
    ASSERT_TRUE(mon_comms<>::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.n_ports, 1U);
    EXPECT_EQ(decoded.tx_errors, 0x02U);
    EXPECT_FALSE(decoded.tx_error_mem());
    EXPECT_TRUE(decoded.tx_error_alloc());
    EXPECT_EQ(decoded.prot_ids[0U], mon_comms_protocol::nmea);
    EXPECT_EQ(decoded.prot_ids[1U], mon_comms_protocol::ubx);
    EXPECT_EQ(decoded.prot_ids[2U], mon_comms_protocol::rtcm2);
    EXPECT_EQ(decoded.prot_ids[3U], mon_comms_protocol::none);
    ASSERT_EQ(decoded.ports.size(), 1U);

    CASTLE_CONST mon_comms_port& port = decoded.ports[0U];
    EXPECT_EQ(port.port_id, 0x0101U);
    EXPECT_EQ(port.tx_pending, 0x1234U);
    EXPECT_EQ(port.tx_bytes, 0x89ABCDEFU);
    EXPECT_EQ(port.tx_usage, 55U);
    EXPECT_EQ(port.tx_peak_usage, 66U);
    EXPECT_EQ(port.rx_pending, 0x4321U);
    EXPECT_EQ(port.rx_bytes, 0x76543210U);
    EXPECT_EQ(port.rx_usage, 77U);
    EXPECT_EQ(port.rx_peak_usage, 88U);
    EXPECT_EQ(port.overrun_errs, 0x0F0FU);
    EXPECT_EQ(port.msgs[0U], 0x1001U);
    EXPECT_EQ(port.msgs[1U], 0x2002U);
    EXPECT_EQ(port.msgs[2U], 0x3003U);
    EXPECT_EQ(port.msgs[3U], 0x4004U);
    EXPECT_EQ(port.skipped, 0xFEEDF00DU);
}

TEST(UbxMessageMonComms, TxErrorAccessorsSetAndClear)
{
    mon_comms<> message{};
    EXPECT_FALSE(message.tx_error_mem());
    EXPECT_FALSE(message.tx_error_alloc());
    message.tx_errors = mon_comms<>::TX_ERRORS_MEM;
    EXPECT_TRUE(message.tx_error_mem());
    EXPECT_FALSE(message.tx_error_alloc());
    message.tx_errors = static_cast<uint8_t>(mon_comms<>::TX_ERRORS_MEM | mon_comms<>::TX_ERRORS_ALLOC);
    EXPECT_TRUE(message.tx_error_mem());
    EXPECT_TRUE(message.tx_error_alloc());
}

TEST(UbxMessageMonComms, ParsedMessagesLooksUpProtocolSlot)
{
    mon_comms<> message{};
    message.prot_ids[0U] = mon_comms_protocol::ubx;
    message.prot_ids[1U] = mon_comms_protocol::nmea;
    message.prot_ids[2U] = mon_comms_protocol::rtcm3;
    CASTLE_CONST mon_comms_port port = distinctive_port(3U);

    uint16_t count = 0xFFFFU;
    ASSERT_TRUE(message.parsed_messages(port, mon_comms_protocol::ubx, count));
    EXPECT_EQ(count, port.msgs[0U]);
    ASSERT_TRUE(message.parsed_messages(port, mon_comms_protocol::rtcm3, count));
    EXPECT_EQ(count, port.msgs[2U]);
    ASSERT_TRUE(message.parsed_messages(port, mon_comms_protocol::none, count));
    EXPECT_EQ(count, port.msgs[3U]);

    count = 0xFFFFU;
    EXPECT_FALSE(message.parsed_messages(port, mon_comms_protocol::spartn, count));
    EXPECT_EQ(count, 0U);
}

TEST(UbxMessageMonComms, HeaderOnlyPayloadDecodesWithNoPorts)
{
    uint8_t payload[8U] = {0x00U, 0U, 0x01U, 0U, 0U, 1U, 5U, 6U};
    mon_comms<> decoded{};
    ASSERT_TRUE(mon_comms<>::decode(view_of(payload, 8U), decoded));
    EXPECT_EQ(decoded.n_ports, 0U);
    EXPECT_TRUE(decoded.ports.empty());
    EXPECT_TRUE(decoded.tx_error_mem());
    EXPECT_EQ(decoded.prot_ids[3U], mon_comms_protocol::spartn);
}

TEST(UbxMessageMonComms, CapacityOverflowKeepsWireCountAndStoresPrefix)
{
    uint8_t payload[8U + (3U * 40U)] = {};
    payload[1U] = 3U;
    for (castle::size_type index = 0U; index < 3U; ++index)
    {
        castle::write_le16(&payload[8U + (index * 40U)], static_cast<uint16_t>(0x0100U + index));
        castle::write_le32(&payload[8U + (index * 40U) + 36U], static_cast<uint32_t>(0xA0U + index));
    }
    mon_comms<1U> decoded{};
    ASSERT_TRUE(mon_comms<1U>::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.n_ports, 3U);
    ASSERT_EQ(decoded.ports.size(), 1U);
    EXPECT_EQ(decoded.ports[0U].port_id, 0x0100U);
    EXPECT_EQ(decoded.ports[0U].skipped, 0xA0U);
}

TEST(UbxMessageMonComms, RejectsGroupLengthMismatch)
{
    uint8_t payload[8U + 81U] = {};
    payload[1U] = 2U;
    payload[2U] = 0x03U;
    mon_comms<> decoded{};
    EXPECT_FALSE(mon_comms<>::decode(view_of(payload, 8U + 40U), decoded));
    EXPECT_EQ(decoded.n_ports, 0U);
    EXPECT_EQ(decoded.tx_errors, 0U);
    EXPECT_EQ(decoded.prot_ids[0U], mon_comms_protocol::none);
    EXPECT_FALSE(mon_comms<>::decode(view_of(payload, 8U + 81U), decoded));
    EXPECT_EQ(decoded.tx_errors, 0U);
}

TEST(UbxMessageMonComms, RejectsShortHeaderAndWrongClassOrId)
{
    uint8_t payload[8U] = {};
    mon_comms<> decoded{};
    EXPECT_FALSE(mon_comms<>::decode(view_of(payload, 7U), decoded));
    message_view empty{{UBX_CLASS_MON, UBX_ID_MON_COMMS, 0U}, array_view<CASTLE_CONST uint8_t>(), checksum{}};
    EXPECT_FALSE(mon_comms<>::decode(empty, decoded));
    EXPECT_FALSE(mon_comms<>::matches(view_of(payload, 8U, UBX_CLASS_NAV, UBX_ID_MON_COMMS)));
    EXPECT_FALSE(mon_comms<>::decode(view_of(payload, 8U, UBX_CLASS_NAV, UBX_ID_MON_COMMS), decoded));
    EXPECT_FALSE(mon_comms<>::matches(view_of(payload, 8U, UBX_CLASS_MON, UBX_ID_MON_IO)));
    EXPECT_FALSE(mon_comms<>::decode(view_of(payload, 8U, UBX_CLASS_MON, UBX_ID_MON_IO), decoded));
}

TEST(UbxMessageMonComms, EncodeIntoTooSmallBufferReportsFull)
{
    mon_comms<> message{};
    ASSERT_EQ(message.ports.push_back(distinctive_port(1U)), castle::status::ok);
    uint8_t frame[frame_size(8U + 39U)] = {};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
