#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mga_gal.hpp"

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

/// Mirrors a decoder registry: counts the MGA-GAL models that claim `raw`.
int claimants(CASTLE_CONST message_view& raw)
{
    return static_cast<int>(mga_gal_eph::matches(raw)) + static_cast<int>(mga_gal_alm::matches(raw))
        + static_cast<int>(mga_gal_timeoffset::matches(raw)) + static_cast<int>(mga_gal_utc::matches(raw));
}
} // namespace

int main()
{
    uint8_t frame[128U];

    mga_gal_eph eph{};
    eph.sv_id = 11U;
    eph.iod_nav = 77U;
    eph.toe = 840U;
    eph.af2 = -1;
    eph.health_e5b = 1U;
    eph.data_validity_e5b = 1U;
    message_view raw = encode_and_parse(eph, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_MGA, UBX_ID_MGA_GAL));
    CASTLE_SAMPLE_CHECK(raw.payload[0U] == MGA_GAL_TYPE_EPH && raw.payload[1U] == MGA_GAL_VERSION);
    CASTLE_SAMPLE_CHECK(mga_gal_eph::matches(raw) && claimants(raw) == 1);
    mga_gal_eph eph_back{};
    CASTLE_SAMPLE_CHECK(mga_gal_eph::decode(raw, eph_back));
    CASTLE_SAMPLE_CHECK(eph_back.iod_nav == 77U && eph_back.toe == 840U && eph_back.af2 == -1);
    CASTLE_SAMPLE_CHECK(eph_back.health_e5b == 1U && eph_back.data_validity_e5b == 1U);

    mga_gal_alm alm{};
    alm.sv_id = 3U;
    alm.ioda = 9U;
    alm.delta_sqrt_a = -55;
    raw = encode_and_parse(alm, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_gal_alm::matches(raw) && claimants(raw) == 1);
    mga_gal_alm alm_back{};
    CASTLE_SAMPLE_CHECK(mga_gal_alm::decode(raw, alm_back));
    CASTLE_SAMPLE_CHECK(alm_back.ioda == 9U && alm_back.delta_sqrt_a == -55);

    mga_gal_timeoffset ggto{};
    ggto.a0_g = -12;
    ggto.a1_g = 3;
    ggto.t0_g = 24U;
    ggto.wn0_g = 63U;
    raw = encode_and_parse(ggto, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_gal_timeoffset::matches(raw) && claimants(raw) == 1);
    mga_gal_timeoffset ggto_back{};
    CASTLE_SAMPLE_CHECK(mga_gal_timeoffset::decode(raw, ggto_back));
    CASTLE_SAMPLE_CHECK(ggto_back.a0_g == -12 && ggto_back.t0_g == 24U && ggto_back.wn0_g == 63U);

    mga_gal_utc utc{};
    utc.dt_ls = 18;
    utc.tot = 10U;
    utc.wnt = 20U;
    raw = encode_and_parse(utc, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_gal_utc::matches(raw) && claimants(raw) == 1);
    mga_gal_utc utc_back{};
    CASTLE_SAMPLE_CHECK(mga_gal_utc::decode(raw, utc_back));
    CASTLE_SAMPLE_CHECK(utc_back.dt_ls == 18 && utc_back.tot == 10U && utc_back.wnt == 20U);

    mga_gal_eph not_eph{};
    CASTLE_SAMPLE_CHECK(!mga_gal_eph::decode(raw, not_eph));

    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(eph, frame, 20U, written) == castle::status::full);

    return 0;
}
