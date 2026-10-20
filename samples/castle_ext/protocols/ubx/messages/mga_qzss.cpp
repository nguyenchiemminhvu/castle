#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mga_qzss.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

template <typename Message>
message_view encode_and_parse(CASTLE_CONST Message& message, uint8_t* frame, castle::size_type capacity)
{
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(message, frame, capacity, written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == Message::payload_length + UBX_FRAME_OVERHEAD);
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    return raw;
}

/// Mirrors a decoder registry holding both the GPS and the QZSS models.
int claimants(CASTLE_CONST message_view& raw)
{
    return static_cast<int>(mga_qzss_eph::matches(raw)) + static_cast<int>(mga_qzss_alm::matches(raw))
        + static_cast<int>(mga_qzss_health::matches(raw)) + static_cast<int>(mga_gps_eph::matches(raw))
        + static_cast<int>(mga_gps_alm::matches(raw)) + static_cast<int>(mga_gps_health::matches(raw));
}
} // namespace

int main()
{
    uint8_t frame[96U];

    // Same layout as GPS, but encoded under the QZSS message ID.
    mga_qzss_eph eph{};
    eph.sv_id = 194U;
    eph.toe = 3600U;
    eph.idot = -42;
    message_view raw = encode_and_parse(eph, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_MGA, UBX_ID_MGA_QZSS));
    CASTLE_SAMPLE_CHECK(raw.payload[0U] == MGA_GPS_TYPE_EPH);
    CASTLE_SAMPLE_CHECK(mga_qzss_eph::matches(raw) && !mga_gps_eph::matches(raw) && claimants(raw) == 1);
    mga_qzss_eph eph_back{};
    CASTLE_SAMPLE_CHECK(mga_qzss_eph::decode(raw, eph_back));
    CASTLE_SAMPLE_CHECK(eph_back.sv_id == 194U && eph_back.toe == 3600U && eph_back.idot == -42);

    mga_qzss_alm alm{};
    alm.sv_id = 195U;
    alm.toa = 100U;
    raw = encode_and_parse(alm, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_qzss_alm::matches(raw) && claimants(raw) == 1);
    mga_qzss_alm alm_back{};
    CASTLE_SAMPLE_CHECK(mga_qzss_alm::decode(raw, alm_back));
    CASTLE_SAMPLE_CHECK(alm_back.sv_id == 195U && alm_back.toa == 100U);

    mga_qzss_health health{};
    health.health_code[0U] = 0x01U;
    health.health_code[mga_qzss_health::NUM_SVS - 1U] = 0x3FU;
    raw = encode_and_parse(health, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_qzss_health::matches(raw) && claimants(raw) == 1);
    mga_qzss_health health_back{};
    CASTLE_SAMPLE_CHECK(mga_qzss_health::decode(raw, health_back));
    CASTLE_SAMPLE_CHECK((health_back.health_code[4U] & mga_qzss_health::HEALTH_CODE_MASK) == 0x3FU);

    // A GPS-ID ephemeris is not a QZSS one.
    mga_gps_eph gps{};
    raw = encode_and_parse(gps, frame, sizeof(frame));
    mga_qzss_eph not_qzss{};
    CASTLE_SAMPLE_CHECK(!mga_qzss_eph::decode(raw, not_qzss) && claimants(raw) == 1);

    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(health, frame, 12U, written) == castle::status::full);

    return 0;
}
