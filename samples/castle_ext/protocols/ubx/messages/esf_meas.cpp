#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/esf_meas.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::messages;

    // Build a UBX-ESF-MEAS payload:
    //
    // time_tag      = 1000 (0x000003E8)
    // flags         = 0 (no calib_ttag field present)
    // id            = 7
    // measurement   = type 11, value 12345

    uint8_t payload[12U];

    write_le32(&payload[0U], 1000U);     // time_tag
    write_le16(&payload[4U], 0U);        // flags
    write_le16(&payload[6U], 7U);        // id

    uint32_t measurement_word =
        (static_cast<uint32_t>(11U) << 24U)
        | 12345U;

    write_le32(&payload[8U], measurement_word);

    castle::container::array_view<const uint8_t> payload_view(
        payload,
        sizeof(payload));

    // Encode a complete UBX frame.
    uint8_t frame[64U];
    castle::size_type written = 0U;

    castle::status encode_status = encode_frame(
        UBX_CLASS_ESF,
        UBX_ID_ESF_MEAS,
        payload_view,
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    // Decode the UBX frame.
    message_view raw;
    bool frame_ok =
        decode_frame(
            castle::container::array_view<const uint8_t>(frame, written),
            raw);

    CASTLE_SAMPLE_CHECK(frame_ok);
    CASTLE_SAMPLE_CHECK(esf_meas<>::matches(raw));

    // Decode the ESF-MEAS payload.
    esf_meas<> message;

    bool decoded = esf_meas<>::decode(raw, message);

    CASTLE_SAMPLE_CHECK(decoded);

    CASTLE_SAMPLE_CHECK(message.time_tag == 1000U);
    CASTLE_SAMPLE_CHECK(message.flags == 0U);
    CASTLE_SAMPLE_CHECK(message.id == 7U);

    CASTLE_SAMPLE_CHECK(message.num_meas == 1U);

    CASTLE_SAMPLE_CHECK(message.data[0U].data_type == 11U);
    CASTLE_SAMPLE_CHECK(message.data[0U].data_value == 12345);

    CASTLE_SAMPLE_CHECK(!message.has_calib_ttag);

    // Example with calibration time tag present.
    uint8_t payload_with_calib[16U];

    write_le32(&payload_with_calib[0U], 2000U); // time_tag
    write_le16(&payload_with_calib[4U], 1U);    // FLAGS_TIME_TAG_TYPE_MASK set
    write_le16(&payload_with_calib[6U], 42U);   // id

    uint32_t measurement_word2 =
        (static_cast<uint32_t>(3U) << 24U)
        | 500U;

    write_le32(&payload_with_calib[8U], measurement_word2);
    write_le32(&payload_with_calib[12U], 9999U); // calib_ttag

    castle::size_type written2 = 0U;

    encode_status = encode_frame(
        UBX_CLASS_ESF,
        UBX_ID_ESF_MEAS,
        castle::container::array_view<const uint8_t>(
            payload_with_calib,
            sizeof(payload_with_calib)),
        frame,
        sizeof(frame),
        written2);

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    frame_ok = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written2),
        raw);

    CASTLE_SAMPLE_CHECK(frame_ok);

    decoded = esf_meas<>::decode(raw, message);

    CASTLE_SAMPLE_CHECK(decoded);
    CASTLE_SAMPLE_CHECK(message.has_calib_ttag);
    CASTLE_SAMPLE_CHECK(message.calib_ttag == 9999U);

    return 0;
}
