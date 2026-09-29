#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/esf_ins.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // ---------------------------------------------------------------------
    // Construct a UBX-ESF-INS payload.
    // ---------------------------------------------------------------------

    uint8_t payload[esf_ins::payload_length];
    castle::size_type offset = 0U;

    castle::write_le32(
        &payload[offset],
        esf_ins::BF0_X_ANG_RATE_VALID
        | esf_ins::BF0_Y_ANG_RATE_VALID
        | esf_ins::BF0_Z_ANG_RATE_VALID
        | esf_ins::BF0_X_ACCEL_VALID
        | esf_ins::BF0_Y_ACCEL_VALID
        | esf_ins::BF0_Z_ACCEL_VALID);
    offset += 4U;

    // Reserved bytes.
    castle::write_le32(&payload[offset], 0U);
    offset += 4U;

    castle::write_le32(&payload[offset], 123456U); // iTOW
    offset += 4U;

    castle::write_le32(&payload[offset], static_cast<uint32_t>(100));
    offset += 4U;

    castle::write_le32(&payload[offset], static_cast<uint32_t>(-200));
    offset += 4U;

    castle::write_le32(&payload[offset], static_cast<uint32_t>(300));
    offset += 4U;

    castle::write_le32(&payload[offset], static_cast<uint32_t>(1000));
    offset += 4U;

    castle::write_le32(&payload[offset], static_cast<uint32_t>(-2000));
    offset += 4U;

    castle::write_le32(&payload[offset], static_cast<uint32_t>(3000));
    offset += 4U;

    CASTLE_SAMPLE_CHECK(offset == esf_ins::payload_length);

    // ---------------------------------------------------------------------
    // Encode UBX frame.
    // ---------------------------------------------------------------------

    uint8_t frame[128U];
    castle::size_type written = 0U;

    castle::status status = encode_frame(
        UBX_CLASS_ESF,
        UBX_ID_ESF_INS,
        castle::container::array_view<const uint8_t>(payload, sizeof(payload)),
        frame,
        sizeof(frame),
        written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(status));

    // ---------------------------------------------------------------------
    // Decode raw UBX frame.
    // ---------------------------------------------------------------------

    message_view raw;

    bool decoded = decode_frame(
        castle::container::array_view<const uint8_t>(frame, written),
        raw);

    CASTLE_SAMPLE_CHECK(decoded);
    CASTLE_SAMPLE_CHECK(esf_ins::matches(raw));

    // ---------------------------------------------------------------------
    // Decode ESF-INS message.
    // ---------------------------------------------------------------------

    esf_ins ins;

    CASTLE_SAMPLE_CHECK(esf_ins::decode(raw, ins));

    CASTLE_SAMPLE_CHECK(
        (ins.bitfield0 & esf_ins::BF0_X_ANG_RATE_VALID) != 0U);
    CASTLE_SAMPLE_CHECK(
        (ins.bitfield0 & esf_ins::BF0_Y_ANG_RATE_VALID) != 0U);
    CASTLE_SAMPLE_CHECK(
        (ins.bitfield0 & esf_ins::BF0_Z_ACCEL_VALID) != 0U);

    CASTLE_SAMPLE_CHECK(ins.i_tow == 123456U);

    CASTLE_SAMPLE_CHECK(ins.x_ang_rate == 100);
    CASTLE_SAMPLE_CHECK(ins.y_ang_rate == -200);
    CASTLE_SAMPLE_CHECK(ins.z_ang_rate == 300);

    CASTLE_SAMPLE_CHECK(ins.x_accel == 1000);
    CASTLE_SAMPLE_CHECK(ins.y_accel == -2000);
    CASTLE_SAMPLE_CHECK(ins.z_accel == 3000);

    return 0;
}