#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mga_gps.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

/// Encodes `message`, re-parses the frame and returns the view (frame storage is caller-owned).
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

/// Mirrors a decoder registry: counts the MGA-GPS models that claim `raw`.
int claimants(CASTLE_CONST message_view& raw)
{
    return static_cast<int>(mga_gps_eph::matches(raw)) + static_cast<int>(mga_gps_alm::matches(raw))
        + static_cast<int>(mga_gps_health::matches(raw)) + static_cast<int>(mga_gps_utc::matches(raw))
        + static_cast<int>(mga_gps_iono::matches(raw));
}
} // namespace

int main()
{
    uint8_t frame[128U];

    // Ephemeris: host-built assistance, sent to the receiver.
    mga_gps_eph eph{};
    eph.sv_id = 7U;
    eph.iodc = 0x1A3U;
    eph.toe = 22500U;
    eph.sqrt_a = 2702126000U;
    eph.e = 85899345U;
    eph.af0 = -12345;
    message_view raw = encode_and_parse(eph, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_MGA, UBX_ID_MGA_GPS));
    CASTLE_SAMPLE_CHECK(raw.payload[0U] == MGA_GPS_TYPE_EPH);
    CASTLE_SAMPLE_CHECK(raw.payload[1U] == MGA_GPS_VERSION);
    CASTLE_SAMPLE_CHECK(mga_gps_eph::matches(raw));
    CASTLE_SAMPLE_CHECK(claimants(raw) == 1);
    mga_gps_eph eph_back{};
    CASTLE_SAMPLE_CHECK(mga_gps_eph::decode(raw, eph_back));
    CASTLE_SAMPLE_CHECK(eph_back.sv_id == 7U && eph_back.iodc == 0x1A3U && eph_back.toe == 22500U);
    CASTLE_SAMPLE_CHECK(eph_back.sqrt_a == 2702126000U && eph_back.e == 85899345U && eph_back.af0 == -12345);

    // A frame of one layout is never decoded by another.
    mga_gps_alm wrong{};
    CASTLE_SAMPLE_CHECK(!mga_gps_alm::decode(raw, wrong));

    // Almanac.
    mga_gps_alm alm{};
    alm.sv_id = 12U;
    alm.alm_wn_a = 200U;
    alm.toa = 147U;
    alm.m0 = -4000000;
    raw = encode_and_parse(alm, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_gps_alm::matches(raw) && claimants(raw) == 1);
    mga_gps_alm alm_back{};
    CASTLE_SAMPLE_CHECK(mga_gps_alm::decode(raw, alm_back));
    CASTLE_SAMPLE_CHECK(alm_back.sv_id == 12U && alm_back.alm_wn_a == 200U && alm_back.m0 == -4000000);

    // Health for all 32 PRNs.
    mga_gps_health health{};
    health.health_code[0U] = 0x00U;
    health.health_code[mga_gps_health::NUM_SVS - 1U] = 0x3FU;
    raw = encode_and_parse(health, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_gps_health::matches(raw) && claimants(raw) == 1);
    mga_gps_health health_back{};
    CASTLE_SAMPLE_CHECK(mga_gps_health::decode(raw, health_back));
    CASTLE_SAMPLE_CHECK(health_back.health_code[31U] == 0x3FU);

    // UTC model.
    mga_gps_utc utc{};
    utc.utc_dt_ls = 18;
    utc.utc_dt_lsf = 18;
    utc.utc_wn_lsf = 137U;
    utc.utc_dn = 7U;
    raw = encode_and_parse(utc, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_gps_utc::matches(raw) && claimants(raw) == 1);
    mga_gps_utc utc_back{};
    CASTLE_SAMPLE_CHECK(mga_gps_utc::decode(raw, utc_back));
    CASTLE_SAMPLE_CHECK(utc_back.utc_dt_ls == 18 && utc_back.utc_wn_lsf == 137U && utc_back.utc_dn == 7U);

    // Klobuchar ionosphere.
    mga_gps_iono iono{};
    iono.iono_alpha[0U] = 11;
    iono.iono_beta[3U] = -2;
    raw = encode_and_parse(iono, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_gps_iono::matches(raw) && claimants(raw) == 1);
    mga_gps_iono iono_back{};
    CASTLE_SAMPLE_CHECK(mga_gps_iono::decode(raw, iono_back));
    CASTLE_SAMPLE_CHECK(iono_back.iono_alpha[0U] == 11 && iono_back.iono_beta[3U] == -2);

    // Too-small output buffer.
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(iono, frame, 10U, written) == castle::status::full);

    return 0;
}
