#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_slas.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a NAV-SLAS value: ground monitoring station near Tokyo, QZS-3 (svId 195).
    nav_slas<4U> slas{};
    slas.i_tow = 86400000U;
    slas.version = 0U;
    slas.gms_lon = 139692; // 139.692 deg
    slas.gms_lat = 35689;  //  35.689 deg
    slas.gms_code = 4U;
    slas.qzss_sv_id = 195U;
    slas.service_flags = nav_slas<>::SERVICE_FLAGS_GMS_AVAILABLE | nav_slas<>::SERVICE_FLAGS_QZSS_SV_AVAILABLE;

    nav_slas_correction gps{};
    gps.gnss_id = 0U;
    gps.sv_id = 17U;
    gps.prc = -87; // -0.87 m
    CASTLE_SAMPLE_CHECK(castle::succeeded(slas.corrections.push_back(gps)));

    nav_slas_correction qzss{};
    qzss.gnss_id = 5U;
    qzss.sv_id = 2U;
    qzss.prc = 140;
    CASTLE_SAMPLE_CHECK(castle::succeeded(slas.corrections.push_back(qzss)));

    // Encode, then parse the frame back.
    uint8_t frame[48U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(slas, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + nav_slas<>::header_length + (2U * nav_slas<>::block_length)));

    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_slas<>::matches(raw));

    nav_slas<> decoded;
    CASTLE_SAMPLE_CHECK(nav_slas<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 86400000U);
    CASTLE_SAMPLE_CHECK(decoded.gms_longitude_deg() > 139.691);
    CASTLE_SAMPLE_CHECK(decoded.gms_longitude_deg() < 139.693);
    CASTLE_SAMPLE_CHECK(decoded.gms_latitude_deg() > 35.688);
    CASTLE_SAMPLE_CHECK(decoded.gms_latitude_deg() < 35.690);
    CASTLE_SAMPLE_CHECK(decoded.gms_code == 4U);
    CASTLE_SAMPLE_CHECK(decoded.qzss_sv_id == 195U);
    CASTLE_SAMPLE_CHECK(decoded.gms_available());
    CASTLE_SAMPLE_CHECK(decoded.qzss_sv_available());
    CASTLE_SAMPLE_CHECK(!decoded.test_mode());
    CASTLE_SAMPLE_CHECK(decoded.cnt == 2U);
    CASTLE_SAMPLE_CHECK(decoded.corrections.size() == 2U);
    CASTLE_SAMPLE_CHECK(decoded.corrections[0U].sv_id == 17U);
    CASTLE_SAMPLE_CHECK(decoded.corrections[0U].prc_m() < -0.86);
    CASTLE_SAMPLE_CHECK(decoded.corrections[0U].prc_m() > -0.88);
    CASTLE_SAMPLE_CHECK(decoded.corrections[1U].gnss_id == 5U);
    CASTLE_SAMPLE_CHECK(decoded.corrections[1U].prc == 140);

    // A smaller capacity keeps the first block and still reports the wire count.
    nav_slas<1U> clipped;
    CASTLE_SAMPLE_CHECK(nav_slas<1U>::decode(raw, clipped));
    CASTLE_SAMPLE_CHECK(clipped.cnt == 2U);
    CASTLE_SAMPLE_CHECK(clipped.corrections.size() == 1U);

    raw.header.msg_id = UBX_ID_NAV_SBAS;
    CASTLE_SAMPLE_CHECK(!nav_slas<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!nav_slas<>::decode(raw, decoded));

    return 0;
}
