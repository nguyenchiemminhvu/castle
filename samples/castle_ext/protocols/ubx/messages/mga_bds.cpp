#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mga_bds.hpp"

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

/// Mirrors a decoder registry: counts the MGA-BDS models that claim `raw`.
int claimants(CASTLE_CONST message_view& raw)
{
    return static_cast<int>(mga_bds_eph::matches(raw)) + static_cast<int>(mga_bds_alm::matches(raw))
        + static_cast<int>(mga_bds_health::matches(raw)) + static_cast<int>(mga_bds_utc::matches(raw))
        + static_cast<int>(mga_bds_iono::matches(raw));
}
} // namespace

int main()
{
    uint8_t frame[128U];

    mga_bds_eph eph{};
    eph.sv_id = 21U;
    eph.iode = 3U;
    eph.toe = 50400U;
    eph.crs = -1234;
    eph.cis = 777;
    message_view raw = encode_and_parse(eph, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_MGA, UBX_ID_MGA_BDS));
    CASTLE_SAMPLE_CHECK(raw.payload[0U] == MGA_BDS_TYPE_EPH && raw.payload[1U] == MGA_BDS_VERSION);
    CASTLE_SAMPLE_CHECK(mga_bds_eph::matches(raw) && claimants(raw) == 1);
    mga_bds_eph eph_back{};
    CASTLE_SAMPLE_CHECK(mga_bds_eph::decode(raw, eph_back));
    CASTLE_SAMPLE_CHECK(eph_back.sv_id == 21U && eph_back.toe == 50400U && eph_back.crs == -1234 && eph_back.cis == 777);

    mga_bds_alm alm{};
    alm.sv_id = 5U;
    alm.wna = 42U;
    alm.a1 = -9;
    raw = encode_and_parse(alm, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_bds_alm::matches(raw) && claimants(raw) == 1);
    mga_bds_alm alm_back{};
    CASTLE_SAMPLE_CHECK(mga_bds_alm::decode(raw, alm_back));
    CASTLE_SAMPLE_CHECK(alm_back.wna == 42U && alm_back.a1 == -9);

    mga_bds_health health{};
    health.health_code[mga_bds_health::NUM_SVS - 1U] = 0x0101U;
    raw = encode_and_parse(health, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_bds_health::matches(raw) && claimants(raw) == 1);
    mga_bds_health health_back{};
    CASTLE_SAMPLE_CHECK(mga_bds_health::decode(raw, health_back));
    CASTLE_SAMPLE_CHECK((health_back.health_code[29U] & mga_bds_health::HEALTH_CODE_MASK) == 0x0101U);

    mga_bds_utc utc{};
    utc.dt_ls = 4;
    utc.dt_lsf = 5;
    utc.wn_rec = 99U;
    raw = encode_and_parse(utc, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_bds_utc::matches(raw) && claimants(raw) == 1);
    mga_bds_utc utc_back{};
    CASTLE_SAMPLE_CHECK(mga_bds_utc::decode(raw, utc_back));
    CASTLE_SAMPLE_CHECK(utc_back.dt_ls == 4 && utc_back.dt_lsf == 5 && utc_back.wn_rec == 99U);

    mga_bds_iono iono{};
    iono.alpha[1U] = -3;
    iono.beta[2U] = 8;
    raw = encode_and_parse(iono, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_bds_iono::matches(raw) && claimants(raw) == 1);
    mga_bds_iono iono_back{};
    CASTLE_SAMPLE_CHECK(mga_bds_iono::decode(raw, iono_back));
    CASTLE_SAMPLE_CHECK(iono_back.alpha[1U] == -3 && iono_back.beta[2U] == 8);

    // The IONO frame is rejected by every other layout's decoder.
    mga_bds_utc not_utc{};
    CASTLE_SAMPLE_CHECK(!mga_bds_utc::decode(raw, not_utc));

    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(eph, frame, 40U, written) == castle::status::full);

    return 0;
}
