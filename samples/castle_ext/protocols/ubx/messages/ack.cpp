#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/ack.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // ACK-ACK for CFG-VALSET.
    uint8_t ack_payload[] =
    {
        UBX_CLASS_CFG,
        UBX_ID_CFG_VALSET
    };

    uint8_t frame[64U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        UBX_CLASS_ACK,
        UBX_ID_ACK_ACK,
        castle::container::array_view<const uint8_t>(
            ack_payload,
            sizeof(ack_payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // Decode raw UBX frame.
    message_view raw;
    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);

    // Parse as ACK-ACK.
    CASTLE_SAMPLE_CHECK(ack_ack::matches(raw));

    ack_ack ack_response;
    CASTLE_SAMPLE_CHECK(ack_ack::decode(raw, ack_response));

    CASTLE_SAMPLE_CHECK(ack_response.cls_id == UBX_CLASS_CFG);
    CASTLE_SAMPLE_CHECK(ack_response.msg_id_ == UBX_ID_CFG_VALSET);

    // Parse through the generic ACK wrapper.
    ack generic_ack;
    CASTLE_SAMPLE_CHECK(ack::matches(raw));
    CASTLE_SAMPLE_CHECK(ack::decode(raw, generic_ack));

    CASTLE_SAMPLE_CHECK(generic_ack.accepted);
    CASTLE_SAMPLE_CHECK(generic_ack.acked_class == UBX_CLASS_CFG);
    CASTLE_SAMPLE_CHECK(generic_ack.acked_id == UBX_ID_CFG_VALSET);

    // ---------------------------------------------------------------------
    // Example NAK.
    // ---------------------------------------------------------------------

    uint8_t nak_payload[] =
    {
        UBX_CLASS_CFG,
        UBX_ID_CFG_VALGET
    };

    written = 0U;

    status = encode_frame(
        UBX_CLASS_ACK,
        UBX_ID_ACK_NAK,
        castle::container::array_view<const uint8_t>(
            nak_payload,
            sizeof(nak_payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);

    ack nak_response;
    CASTLE_SAMPLE_CHECK(ack::decode(raw, nak_response));

    CASTLE_SAMPLE_CHECK(!nak_response.accepted);
    CASTLE_SAMPLE_CHECK(nak_response.acked_class == UBX_CLASS_CFG);
    CASTLE_SAMPLE_CHECK(nak_response.acked_id == UBX_ID_CFG_VALGET);

    return 0;
}
