#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/rxm_rtcm.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Status of an RTCM 1077 message from reference station 42 that passed CRC and was used.
    rxm_rtcm status_msg{};
    status_msg.version = 0x02U;
    status_msg.flags = static_cast<uint8_t>(static_cast<uint8_t>(rxm_rtcm_msg_used::used) << 1U);
    status_msg.sub_type = 0U;
    status_msg.ref_station = 42U;
    status_msg.msg_type = 1077U;

    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_rtcm::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(status_msg, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(rxm_rtcm::matches(raw));

    rxm_rtcm decoded{};
    CASTLE_SAMPLE_CHECK(rxm_rtcm::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 0x02U);
    CASTLE_SAMPLE_CHECK(decoded.ref_station == 42U);
    CASTLE_SAMPLE_CHECK(decoded.msg_type == 1077U);
    CASTLE_SAMPLE_CHECK(!decoded.crc_failed());
    CASTLE_SAMPLE_CHECK(decoded.msg_used() == rxm_rtcm_msg_used::used);

    // A CRC failure: refStation is not reported for messages without DF003.
    rxm_rtcm failed{};
    failed.flags = rxm_rtcm::FLAGS_CRC_FAILED;
    failed.ref_station = rxm_rtcm::REF_STATION_NOT_REPORTED;
    CASTLE_SAMPLE_CHECK(failed.crc_failed());
    CASTLE_SAMPLE_CHECK(failed.msg_used() == rxm_rtcm_msg_used::unknown);

    // A different RXM message is not RXM-RTCM.
    message_view other = raw;
    other.header.msg_id = UBX_ID_RXM_RAWX;
    CASTLE_SAMPLE_CHECK(!rxm_rtcm::matches(other));
    CASTLE_SAMPLE_CHECK(!rxm_rtcm::decode(other, decoded));

    return 0;
}
