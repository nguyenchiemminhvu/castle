#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mga_ack.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Receiver output: "MGA-GPS-EPH rejected, time unknown".
    uint8_t payload[mga_ack::payload_length] = {};
    payload[0U] = 0x00U;            // type: not used
    payload[1U] = MGA_ACK_VERSION;  // version
    payload[2U] = 0x01U;            // infoCode: no time
    payload[3U] = UBX_ID_MGA_GPS;   // msgId
    payload[4U] = 0x01U;            // first payload bytes of the acknowledged message
    payload[5U] = 0x00U;
    payload[6U] = 0x07U;

    uint8_t frame[32U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_frame(mga_ack::msg_class, mga_ack::msg_id,
                                     castle::container::array_view<const uint8_t>(payload, sizeof(payload)),
                                     frame, sizeof(frame), written) == castle::status::ok);

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mga_ack::matches(raw));

    mga_ack ack{};
    CASTLE_SAMPLE_CHECK(mga_ack::decode(raw, ack));
    CASTLE_SAMPLE_CHECK(!ack.accepted());
    CASTLE_SAMPLE_CHECK(ack.type == mga_ack_type::not_used);
    CASTLE_SAMPLE_CHECK(ack.info_code == mga_ack_info_code::no_time);
    CASTLE_SAMPLE_CHECK(ack.acked_msg_id == UBX_ID_MGA_GPS);
    CASTLE_SAMPLE_CHECK(ack.msg_payload_start[0U] == 0x01U && ack.msg_payload_start[2U] == 0x07U);

    // ACK that ends a UBX-MGA-DBD poll: msgPayloadStart carries the entry count.
    mga_ack done{};
    done.type = mga_ack_type::accepted;
    done.info_code = mga_ack_info_code::accepted;
    done.acked_msg_id = UBX_ID_MGA_DBD;
    done.msg_payload_start[0U] = 0x2AU;
    CASTLE_SAMPLE_CHECK(encode_message(done, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    mga_ack done_back{};
    CASTLE_SAMPLE_CHECK(mga_ack::decode(raw, done_back));
    CASTLE_SAMPLE_CHECK(done_back.accepted());
    CASTLE_SAMPLE_CHECK(done_back.dbd_entry_count() == 42U);

    // A version other than 0x00 is rejected.
    payload[1U] = 0x01U;
    message_view bad{{UBX_CLASS_MGA, UBX_ID_MGA_ACK, mga_ack::payload_length},
                     castle::container::array_view<const uint8_t>(payload, sizeof(payload)), checksum{}};
    CASTLE_SAMPLE_CHECK(mga_ack::matches(bad));
    CASTLE_SAMPLE_CHECK(!mga_ack::decode(bad, ack));

    // Other classes do not match.
    message_view other{};
    other.header.msg_class = UBX_CLASS_ACK;
    other.header.msg_id = UBX_ID_ACK_ACK;
    CASTLE_SAMPLE_CHECK(!mga_ack::matches(other));

    CASTLE_SAMPLE_CHECK(encode_message(done, frame, 10U, written) == castle::status::full);

    return 0;
}
