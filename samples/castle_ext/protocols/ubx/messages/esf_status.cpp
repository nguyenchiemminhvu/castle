#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/esf_status.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a UBX-ESF-STATUS payload with two sensor status records.
    //
    // i_tow       = 123456
    // version     = 2
    // fusion_mode = 1
    // num_sens    = 2

    uint8_t payload[24U] = {};

    castle::write_le32(&payload[0U], 123456U); // i_tow

    payload[4U] = 2U; // version

    // bytes 5..11 reserved

    payload[12U] = 1U; // fusion_mode

    // bytes 13..14 reserved

    payload[15U] = 2U; // num_sens

    // Sensor 0
    payload[16U] = static_cast<uint8_t>(
        esf_status_sensor::USED |
        esf_status_sensor::READY |
        3U);
    payload[17U] = 0x02U;
    payload[18U] = 50U;
    payload[19U] = 0U;

    // Sensor 1
    payload[20U] = static_cast<uint8_t>(
        esf_status_sensor::USED |
        7U);
    payload[21U] = 0x04U;
    payload[22U] = 100U;
    payload[23U] = 1U;

    castle::container::array_view<const uint8_t> payload_view(
        payload,
        sizeof(payload));

    // Encode a UBX frame.
    uint8_t frame[64U];
    castle::size_type written = 0U;

    castle::status encode_status = encode_frame(
        UBX_CLASS_ESF,
        UBX_ID_ESF_STATUS,
        payload_view,
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    // Decode the frame back into a message view.
    message_view raw;
    bool frame_ok =
        decode_frame(
            castle::container::array_view<const uint8_t>(frame, written),
            raw);

    CASTLE_SAMPLE_CHECK(frame_ok);
    CASTLE_SAMPLE_CHECK(esf_status<>::matches(raw));

    // Decode the ESF-STATUS payload.
    esf_status<> status_message;

    bool decoded =
        esf_status<>::decode(raw, status_message);

    CASTLE_SAMPLE_CHECK(decoded);

    CASTLE_SAMPLE_CHECK(status_message.i_tow == 123456U);
    CASTLE_SAMPLE_CHECK(status_message.version == 2U);
    CASTLE_SAMPLE_CHECK(status_message.fusion_mode == 1U);
    CASTLE_SAMPLE_CHECK(status_message.num_sens == 2U);

    // Sensor 0 verification.
    CASTLE_SAMPLE_CHECK(
        status_message.sensors[0U].sens_status1 == (esf_status_sensor::USED | esf_status_sensor::READY | 3U));

    CASTLE_SAMPLE_CHECK(
        status_message.sensors[0U].sens_status2 == 0x02U);

    CASTLE_SAMPLE_CHECK(
        status_message.sensors[0U].freq == 50U);

    CASTLE_SAMPLE_CHECK(
        status_message.sensors[0U].faults == 0U);

    // Sensor 1 verification.
    CASTLE_SAMPLE_CHECK(
        status_message.sensors[1U].sens_status1 == (esf_status_sensor::USED | 7U));

    CASTLE_SAMPLE_CHECK(
        status_message.sensors[1U].sens_status2 == 0x04U);

    CASTLE_SAMPLE_CHECK(
        status_message.sensors[1U].freq == 100U);

    CASTLE_SAMPLE_CHECK(
        status_message.sensors[1U].faults == 1U);

    // Demonstrate inspection of status flag bits.
    CASTLE_SAMPLE_CHECK(
        (status_message.sensors[0U].sens_status1 &
         esf_status_sensor::USED) != 0U);

    CASTLE_SAMPLE_CHECK(
        (status_message.sensors[0U].sens_status1 &
         esf_status_sensor::READY) != 0U);

    return 0;
}
