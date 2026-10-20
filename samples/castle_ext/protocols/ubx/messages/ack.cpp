#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/ack.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Simulate the receiver acknowledging a UBX-CFG-VALSET.
    ack_ack acknowledged{};
    acknowledged.cls_id = UBX_CLASS_CFG;
    acknowledged.acked_msg_id = UBX_ID_CFG_VALSET;

    uint8_t frame[UBX_FRAME_OVERHEAD + ack_ack::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(acknowledged, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(ack_ack::matches(raw));
    CASTLE_SAMPLE_CHECK(!ack_nak::matches(raw));
    CASTLE_SAMPLE_CHECK(ack::matches(raw));

    // Typed model: one struct per ID.
    ack_ack decoded{};
    CASTLE_SAMPLE_CHECK(ack_ack::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.cls_id == UBX_CLASS_CFG);
    CASTLE_SAMPLE_CHECK(decoded.acked_msg_id == UBX_ID_CFG_VALSET);

    // Aggregate model: either ID, `accepted` tells which.
    ack reply{};
    CASTLE_SAMPLE_CHECK(ack::decode(raw, reply));
    CASTLE_SAMPLE_CHECK(reply.accepted);
    CASTLE_SAMPLE_CHECK(reply.acked_class == UBX_CLASS_CFG);
    CASTLE_SAMPLE_CHECK(reply.acked_id == UBX_ID_CFG_VALSET);

    // A rejection of the same message.
    ack_nak rejected{};
    rejected.cls_id = UBX_CLASS_CFG;
    rejected.acked_msg_id = UBX_ID_CFG_VALSET;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(rejected, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(ack_nak::matches(raw));
    CASTLE_SAMPLE_CHECK(!ack_ack::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(ack::decode(raw, reply));
    CASTLE_SAMPLE_CHECK(!reply.accepted);
    CASTLE_SAMPLE_CHECK(reply.acked_id == UBX_ID_CFG_VALSET);

    return 0;
}
