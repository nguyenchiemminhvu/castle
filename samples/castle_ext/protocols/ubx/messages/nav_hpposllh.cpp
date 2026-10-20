#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_hpposllh.hpp"

namespace
{
bool near(double value, double expected, double tolerance)
{
    const double difference = value - expected;
    return (difference <= tolerance) && (difference >= -tolerance);
}
} // namespace

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // lon = 8.512345678 deg: 85123456 * 1e-7 + 78 * 1e-9.
    nav_hpposllh position{};
    position.version = 0U;
    position.flags = 0U;
    position.i_tow = 345600000U;
    position.lon = 85123456;
    position.lat = 472500001;
    position.height = 512345;
    position.h_msl = 464012;
    position.lon_hp = 78;
    position.lat_hp = -50;
    position.height_hp = 7;
    position.h_msl_hp = -3;
    position.h_acc = 140U;  // 14.0 mm
    position.v_acc = 215U;  // 21.5 mm

    uint8_t frame[UBX_FRAME_OVERHEAD + nav_hpposllh::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(position, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_hpposllh::matches(raw));

    nav_hpposllh decoded{};
    CASTLE_SAMPLE_CHECK(nav_hpposllh::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 0U);
    CASTLE_SAMPLE_CHECK(decoded.flags == 0U);
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.lon == 85123456);
    CASTLE_SAMPLE_CHECK(decoded.lat == 472500001);
    CASTLE_SAMPLE_CHECK(decoded.height == 512345);
    CASTLE_SAMPLE_CHECK(decoded.h_msl == 464012);
    CASTLE_SAMPLE_CHECK(decoded.lon_hp == 78);
    CASTLE_SAMPLE_CHECK(decoded.lat_hp == -50);
    CASTLE_SAMPLE_CHECK(decoded.height_hp == 7);
    CASTLE_SAMPLE_CHECK(decoded.h_msl_hp == -3);
    CASTLE_SAMPLE_CHECK(decoded.h_acc == 140U);
    CASTLE_SAMPLE_CHECK(decoded.v_acc == 215U);

    // Combined standard + high-precision parts.
    CASTLE_SAMPLE_CHECK(!decoded.invalid_llh());
    CASTLE_SAMPLE_CHECK(near(decoded.longitude_deg(), 8.512345678, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.latitude_deg(), 47.25000005, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.height_m(), 512.3457, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.h_msl_m(), 464.0117, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.h_acc_m(), 0.014, 1e-12));
    CASTLE_SAMPLE_CHECK(near(decoded.v_acc_m(), 0.0215, 1e-12));

    decoded.flags = nav_hpposllh::FLAGS_INVALID_LLH;
    CASTLE_SAMPLE_CHECK(decoded.invalid_llh());

    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_POSLLH;
    CASTLE_SAMPLE_CHECK(!nav_hpposllh::matches(other));

    return 0;
}
