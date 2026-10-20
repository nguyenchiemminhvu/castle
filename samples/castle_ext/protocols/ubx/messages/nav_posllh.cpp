#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_posllh.hpp"

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

    // Build a NAV-POSLLH solution: 8.5 deg E, 47.25 deg N, 512.345 m ellipsoidal height.
    nav_posllh position{};
    position.i_tow = 345600000U;
    position.lon = 85000000;
    position.lat = 472500000;
    position.height = 512345;
    position.h_msl = 464012;
    position.h_acc = 1500U;
    position.v_acc = 2300U;

    // Encode a complete UBX frame (header + payload + checksum) in place.
    uint8_t frame[UBX_FRAME_OVERHEAD + nav_posllh::payload_length];
    castle::size_type written = 0U;
    const castle::status status = encode_message(position, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(status));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    // Validate the frame and identify it.
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_posllh::matches(raw));
    CASTLE_SAMPLE_CHECK(raw.payload_length() == nav_posllh::payload_length);

    // Decode the payload back into a typed model.
    nav_posllh decoded{};
    CASTLE_SAMPLE_CHECK(nav_posllh::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.lon == 85000000);
    CASTLE_SAMPLE_CHECK(decoded.lat == 472500000);
    CASTLE_SAMPLE_CHECK(decoded.height == 512345);
    CASTLE_SAMPLE_CHECK(decoded.h_msl == 464012);
    CASTLE_SAMPLE_CHECK(decoded.h_acc == 1500U);
    CASTLE_SAMPLE_CHECK(decoded.v_acc == 2300U);

    // Scaled helpers convert the raw integers to SI-style units.
    CASTLE_SAMPLE_CHECK(near(decoded.longitude_deg(), 8.5, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.latitude_deg(), 47.25, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.height_m(), 512.345, 1e-9));
    CASTLE_SAMPLE_CHECK(near(decoded.h_msl_m(), 464.012, 1e-9));

    // A different message is not NAV-POSLLH.
    message_view other = raw;
    other.header.msg_id = UBX_ID_NAV_POSECEF;
    CASTLE_SAMPLE_CHECK(!nav_posllh::matches(other));
    CASTLE_SAMPLE_CHECK(!nav_posllh::decode(other, decoded));

    return 0;
}
