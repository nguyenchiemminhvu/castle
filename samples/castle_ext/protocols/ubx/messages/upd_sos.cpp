#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/upd_sos.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    castle::size_type written = 0U;
    message_view raw{};

    // Before power-off: ask the receiver to save its BBR into flash.
    uint8_t save_frame[UBX_FRAME_OVERHEAD + upd_sos_save::payload_length];
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(upd_sos_save{}, save_frame, sizeof(save_frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(save_frame, written), raw));
    CASTLE_SAMPLE_CHECK(upd_sos_save::matches(raw));
    CASTLE_SAMPLE_CHECK(!upd_sos_clear::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload[0U] == static_cast<uint8_t>(upd_sos_cmd::save));

    // The receiver acknowledges the backup.
    upd_sos_ack ack{};
    ack.response = upd_sos_ack_response::acknowledged;
    uint8_t ack_frame[UBX_FRAME_OVERHEAD + upd_sos_ack::payload_length];
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(ack, ack_frame, sizeof(ack_frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(ack_frame, written), raw));
    CASTLE_SAMPLE_CHECK(upd_sos_ack::matches(raw));
    CASTLE_SAMPLE_CHECK(!upd_sos_restored::matches(raw));

    upd_sos_ack decoded_ack{};
    CASTLE_SAMPLE_CHECK(upd_sos_ack::decode(raw, decoded_ack));
    CASTLE_SAMPLE_CHECK(decoded_ack.response == upd_sos_ack_response::acknowledged);

    // After power-on: poll the restore status (empty payload).
    uint8_t poll[UBX_FRAME_OVERHEAD];
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_poll<upd_sos_restored>(poll, sizeof(poll), written)));
    CASTLE_SAMPLE_CHECK(written == UBX_FRAME_OVERHEAD);

    // The receiver reports that it restored from the backup.
    upd_sos_restored restored{};
    restored.response = upd_sos_restore_response::restored;
    uint8_t restored_frame[UBX_FRAME_OVERHEAD + upd_sos_restored::payload_length];
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(restored, restored_frame, sizeof(restored_frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(restored_frame, written), raw));

    upd_sos_restored decoded_restored{};
    CASTLE_SAMPLE_CHECK(upd_sos_restored::decode(raw, decoded_restored));
    CASTLE_SAMPLE_CHECK(decoded_restored.response == upd_sos_restore_response::restored);
    CASTLE_SAMPLE_CHECK(!upd_sos_ack::decode(raw, decoded_ack));

    // Then clear the backup file.
    uint8_t clear_frame[UBX_FRAME_OVERHEAD + upd_sos_clear::payload_length];
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(upd_sos_clear{}, clear_frame, sizeof(clear_frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(clear_frame, written), raw));
    CASTLE_SAMPLE_CHECK(upd_sos_clear::matches(raw));
    upd_sos_clear decoded_clear{};
    CASTLE_SAMPLE_CHECK(upd_sos_clear::decode(raw, decoded_clear));

    return 0;
}
