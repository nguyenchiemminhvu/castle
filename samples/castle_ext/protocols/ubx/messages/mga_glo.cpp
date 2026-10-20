#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mga_glo.hpp"

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

/// Mirrors a decoder registry: counts the MGA-GLO models that claim `raw`.
int claimants(CASTLE_CONST message_view& raw)
{
    return static_cast<int>(mga_glo_eph::matches(raw)) + static_cast<int>(mga_glo_alm::matches(raw))
        + static_cast<int>(mga_glo_timeoffset::matches(raw));
}
} // namespace

int main()
{
    uint8_t frame[96U];

    mga_glo_eph eph{};
    eph.sv_id = 4U;
    eph.m = 1U;
    eph.h = MGA_GLO_FREQ_CHANNEL_UNKNOWN;
    eph.x = -20000000;
    eph.dz = 5000;
    eph.tau = -77;
    message_view raw = encode_and_parse(eph, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_MGA, UBX_ID_MGA_GLO));
    CASTLE_SAMPLE_CHECK(raw.payload[0U] == MGA_GLO_TYPE_EPH && raw.payload[1U] == MGA_GLO_VERSION);
    CASTLE_SAMPLE_CHECK(mga_glo_eph::matches(raw) && claimants(raw) == 1);
    mga_glo_eph eph_back{};
    CASTLE_SAMPLE_CHECK(mga_glo_eph::decode(raw, eph_back));
    CASTLE_SAMPLE_CHECK(eph_back.h == -128 && eph_back.x == -20000000 && eph_back.dz == 5000 && eph_back.tau == -77);

    mga_glo_alm alm{};
    alm.sv_id = 9U;
    alm.n = 1100U;
    alm.c = 1U;
    alm.h = -7;
    raw = encode_and_parse(alm, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_glo_alm::matches(raw) && claimants(raw) == 1);
    mga_glo_alm alm_back{};
    CASTLE_SAMPLE_CHECK(mga_glo_alm::decode(raw, alm_back));
    CASTLE_SAMPLE_CHECK(alm_back.n == 1100U && alm_back.c == 1U && alm_back.h == -7);

    mga_glo_timeoffset offsets{};
    offsets.n = 1200U;
    offsets.tau_c = -100;
    offsets.tau_gps = 250;
    raw = encode_and_parse(offsets, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_glo_timeoffset::matches(raw) && claimants(raw) == 1);
    mga_glo_timeoffset offsets_back{};
    CASTLE_SAMPLE_CHECK(mga_glo_timeoffset::decode(raw, offsets_back));
    CASTLE_SAMPLE_CHECK(offsets_back.n == 1200U && offsets_back.tau_c == -100 && offsets_back.tau_gps == 250);

    mga_glo_alm not_alm{};
    CASTLE_SAMPLE_CHECK(!mga_glo_alm::decode(raw, not_alm));

    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(eph, frame, 20U, written) == castle::status::full);

    return 0;
}
