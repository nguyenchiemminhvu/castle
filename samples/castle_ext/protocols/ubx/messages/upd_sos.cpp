#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/upd_sos.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // Build a UBX-UPD-SOS response payload.
    uint8_t payload[upd_sos_output::payload_length] = {};

    payload[0U] = UPD_SOS_CMD_ACK; // cmd

    // bytes 1..3 reserved

    payload[4U] = UPD_SOS_RESP_ACK; // response

    // bytes 5..7 reserved

    // Encode payload into a UBX frame.
    uint8_t frame[64U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        upd_sos_output::msg_class,
        upd_sos_output::msg_id,
        castle::container::array_view<const uint8_t>(payload, sizeof(payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // Decode the frame into a generic UBX message view.
    message_view raw;
    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);

    // Verify message identity.
    CASTLE_SAMPLE_CHECK(upd_sos_output::matches(raw));

    // Decode UPD-SOS payload.
    upd_sos_output sos;
    CASTLE_SAMPLE_CHECK(upd_sos_output::decode(raw, sos));

    // Verify decoded fields.
    CASTLE_SAMPLE_CHECK(sos.cmd == UPD_SOS_CMD_ACK);
    CASTLE_SAMPLE_CHECK(sos.response == UPD_SOS_RESP_ACK);

    // RESTORE response is also a valid output message.
    uint8_t restore_payload[upd_sos_output::payload_length] = {};

    restore_payload[0U] = UPD_SOS_CMD_RESTORE;
    restore_payload[4U] = UPD_SOS_RESP_NOT_ACK;

    castle::status restore_status = encode_frame(
        upd_sos_output::msg_class,
        upd_sos_output::msg_id,
        castle::container::array_view<const uint8_t>(
            restore_payload,
            sizeof(restore_payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(restore_status));

    decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);

    upd_sos_output restore;
    CASTLE_SAMPLE_CHECK(upd_sos_output::decode(raw, restore));
    CASTLE_SAMPLE_CHECK(restore.cmd == UPD_SOS_CMD_RESTORE);
    CASTLE_SAMPLE_CHECK(restore.response == UPD_SOS_RESP_NOT_ACK);

    return 0;
}
