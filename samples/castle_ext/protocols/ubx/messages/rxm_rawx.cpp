#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/rxm_rawx.hpp"

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

    // An epoch with two measurements: GPS L1C/A G05 and Galileo E1C E11.
    rxm_rawx<4U> epoch{};
    epoch.rcv_tow = 345600.0005;
    epoch.week = 2300U;
    epoch.leap_s = 18;
    epoch.rec_stat = rxm_rawx<4U>::REC_STAT_LEAP_SEC;
    epoch.version = 0x01U;

    rxm_rawx_meas gps{};
    gps.pr_mes = 21234567.125;
    gps.cp_mes = 111589123.5;
    gps.do_mes = -1234.5F;
    gps.gnss_id = 0U;
    gps.sv_id = 5U;
    gps.locktime = 64500U;
    gps.cno = 45U;
    gps.pr_stdev = 3U;  // 0.01 * 2^3 = 0.08 m
    gps.cp_stdev = 2U;  // 0.004 * 2 = 0.008 cycles
    gps.do_stdev = 4U;  // 0.002 * 2^4 = 0.032 Hz
    gps.trk_stat = rxm_rawx_meas::TRK_STAT_PR_VALID | rxm_rawx_meas::TRK_STAT_CP_VALID
                 | rxm_rawx_meas::TRK_STAT_HALF_CYC;
    CASTLE_SAMPLE_CHECK(epoch.meas.push_back(gps) == castle::status::ok);

    rxm_rawx_meas gal = gps;
    gal.gnss_id = 2U;
    gal.sv_id = 11U;
    gal.sig_id = 1U;
    gal.cp_stdev = rxm_rawx_meas::CP_STD_INVALID;
    gal.trk_stat = rxm_rawx_meas::TRK_STAT_PR_VALID;
    CASTLE_SAMPLE_CHECK(epoch.meas.push_back(gal) == castle::status::ok);

    // encode() writes numMeas from meas.size().
    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_rawx<4U>::header_length + 2U * rxm_rawx<4U>::block_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(epoch, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(rxm_rawx<4U>::matches(raw));

    rxm_rawx<4U> decoded{};
    CASTLE_SAMPLE_CHECK(rxm_rawx<4U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.num_meas == 2U);
    CASTLE_SAMPLE_CHECK(decoded.meas.size() == 2U);
    CASTLE_SAMPLE_CHECK(decoded.rcv_tow == 345600.0005);
    CASTLE_SAMPLE_CHECK(decoded.leap_s == 18);
    CASTLE_SAMPLE_CHECK(decoded.leap_sec());
    CASTLE_SAMPLE_CHECK(!decoded.clk_reset());

    const rxm_rawx_meas& first = decoded.meas[0U];
    CASTLE_SAMPLE_CHECK(first.pr_mes == 21234567.125);
    CASTLE_SAMPLE_CHECK(first.do_mes == -1234.5F);
    CASTLE_SAMPLE_CHECK(first.pr_valid() && first.cp_valid() && first.half_cyc() && !first.sub_half_cyc());
    CASTLE_SAMPLE_CHECK(near(first.pr_std_m(), 0.08, 1e-12));
    CASTLE_SAMPLE_CHECK(near(first.cp_std_cycles(), 0.008, 1e-12));
    CASTLE_SAMPLE_CHECK(near(first.do_std_hz(), 0.032, 1e-12));
    CASTLE_SAMPLE_CHECK(first.cp_std_valid());

    const rxm_rawx_meas& second = decoded.meas[1U];
    CASTLE_SAMPLE_CHECK(second.gnss_id == 2U);
    CASTLE_SAMPLE_CHECK(!second.cp_std_valid());
    CASTLE_SAMPLE_CHECK(!second.cp_valid());

    // A smaller model keeps the first block only but still reports the wire count.
    rxm_rawx<1U> clipped{};
    CASTLE_SAMPLE_CHECK(rxm_rawx<1U>::decode(raw, clipped));
    CASTLE_SAMPLE_CHECK(clipped.num_meas == 2U);
    CASTLE_SAMPLE_CHECK(clipped.meas.size() == 1U);

    return 0;
}
