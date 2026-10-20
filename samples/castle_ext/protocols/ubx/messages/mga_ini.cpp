#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mga_ini.hpp"

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

/// Mirrors a decoder registry: counts the MGA-INI models that claim `raw`.
int claimants(CASTLE_CONST message_view& raw)
{
    return static_cast<int>(mga_ini_pos_xyz::matches(raw)) + static_cast<int>(mga_ini_pos_llh::matches(raw))
        + static_cast<int>(mga_ini_time_utc::matches(raw)) + static_cast<int>(mga_ini_time_gnss::matches(raw))
        + static_cast<int>(mga_ini_clkd::matches(raw)) + static_cast<int>(mga_ini_freq::matches(raw));
}

bool near(double a, double b)
{
    return ((a - b) < 1e-9) && ((b - a) < 1e-9);
}
} // namespace

int main()
{
    uint8_t frame[64U];

    mga_ini_pos_xyz xyz{};
    xyz.ecef_x = 427125000;
    xyz.ecef_y = 70000000;
    xyz.ecef_z = 468000000;
    xyz.pos_acc = 10000U;
    message_view raw = encode_and_parse(xyz, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_MGA, UBX_ID_MGA_INI) && raw.payload[0U] == MGA_INI_TYPE_POS_XYZ);
    CASTLE_SAMPLE_CHECK(mga_ini_pos_xyz::matches(raw) && claimants(raw) == 1);
    mga_ini_pos_xyz xyz_back{};
    CASTLE_SAMPLE_CHECK(mga_ini_pos_xyz::decode(raw, xyz_back));
    CASTLE_SAMPLE_CHECK(near(xyz_back.ecef_x_m(), 4271250.0) && near(xyz_back.ecef_y_m(), 700000.0));
    CASTLE_SAMPLE_CHECK(near(xyz_back.ecef_z_m(), 4680000.0) && near(xyz_back.pos_acc_m(), 100.0));

    // Same 20-byte length as POS_XYZ; only the type byte tells them apart.
    mga_ini_pos_llh llh{};
    llh.lat = 473977000;
    llh.lon = 85456000;
    llh.alt = 41050;
    llh.pos_acc = 2000U;
    raw = encode_and_parse(llh, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_ini_pos_llh::matches(raw) && !mga_ini_pos_xyz::matches(raw) && claimants(raw) == 1);
    mga_ini_pos_llh llh_back{};
    CASTLE_SAMPLE_CHECK(mga_ini_pos_llh::decode(raw, llh_back));
    CASTLE_SAMPLE_CHECK(near(llh_back.latitude_deg(), 47.3977) && near(llh_back.longitude_deg(), 8.5456));
    CASTLE_SAMPLE_CHECK(near(llh_back.alt_m(), 410.5) && near(llh_back.pos_acc_m(), 20.0));

    mga_ini_time_utc utc{};
    utc.ref = static_cast<uint8_t>(static_cast<uint8_t>(mga_ini_time_source::extint0) | mga_ini_time_utc::REF_FALL);
    utc.leap_secs = mga_ini_time_utc::LEAP_SECS_UNKNOWN;
    utc.year = 2026U;
    utc.month = 10U;
    utc.day = 20U;
    utc.hour = 12U;
    utc.ns = 500000000U;
    utc.t_acc_s = 2U;
    raw = encode_and_parse(utc, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_ini_time_utc::matches(raw) && claimants(raw) == 1);
    mga_ini_time_utc utc_back{};
    CASTLE_SAMPLE_CHECK(mga_ini_time_utc::decode(raw, utc_back));
    CASTLE_SAMPLE_CHECK(utc_back.source() == mga_ini_time_source::extint0 && utc_back.fall() && !utc_back.last());
    CASTLE_SAMPLE_CHECK(utc_back.year == 2026U && utc_back.leap_secs == -128 && utc_back.ns == 500000000U);

    mga_ini_time_gnss gnss{};
    gnss.ref = mga_ini_time_gnss::REF_LAST;
    gnss.gnss_id = mga_ini_gnss_id::galileo;
    gnss.week = 1340U;
    gnss.tow = 302400U;
    raw = encode_and_parse(gnss, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_ini_time_gnss::matches(raw) && claimants(raw) == 1);
    mga_ini_time_gnss gnss_back{};
    CASTLE_SAMPLE_CHECK(mga_ini_time_gnss::decode(raw, gnss_back));
    CASTLE_SAMPLE_CHECK(gnss_back.source() == mga_ini_time_source::none && gnss_back.last() && !gnss_back.fall());
    CASTLE_SAMPLE_CHECK(gnss_back.gnss_id == mga_ini_gnss_id::galileo && gnss_back.tow == 302400U);

    mga_ini_clkd clkd{};
    clkd.clk_d = -150;
    clkd.clk_d_acc = 20U;
    raw = encode_and_parse(clkd, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_ini_clkd::matches(raw) && claimants(raw) == 1);
    mga_ini_clkd clkd_back{};
    CASTLE_SAMPLE_CHECK(mga_ini_clkd::decode(raw, clkd_back));
    CASTLE_SAMPLE_CHECK(clkd_back.clk_d == -150 && clkd_back.clk_d_acc == 20U);

    mga_ini_freq freq{};
    freq.flags = static_cast<uint8_t>(static_cast<uint8_t>(mga_ini_freq_source::extint1) | mga_ini_freq::FLAGS_FALL);
    freq.freq = 1000000000;
    freq.freq_acc = 50U;
    raw = encode_and_parse(freq, frame, sizeof(frame));
    CASTLE_SAMPLE_CHECK(mga_ini_freq::matches(raw) && claimants(raw) == 1);
    mga_ini_freq freq_back{};
    CASTLE_SAMPLE_CHECK(mga_ini_freq::decode(raw, freq_back));
    CASTLE_SAMPLE_CHECK(freq_back.source() == mga_ini_freq_source::extint1 && freq_back.fall());
    CASTLE_SAMPLE_CHECK(near(freq_back.freq_hz(), 10000000.0) && freq_back.freq_acc == 50U);

    mga_ini_clkd not_clkd{};
    CASTLE_SAMPLE_CHECK(!mga_ini_clkd::decode(raw, not_clkd));

    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(utc, frame, 20U, written) == castle::status::full);

    return 0;
}
