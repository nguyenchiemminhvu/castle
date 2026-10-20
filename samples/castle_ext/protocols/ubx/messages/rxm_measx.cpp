#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/rxm_measx.hpp"

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

    using measx = rxm_measx<4U>;

    // Header with GPS reference time set and one GPS satellite block.
    measx report{};
    report.version = 0x01U;
    report.gps_tow = 345600000U;
    report.glo_tow = 10800000U;
    report.bds_tow = 345586000U;
    report.qzss_tow = 345600000U;
    report.gps_tow_acc = 8U;  // 8 * 2^-4 = 0.5 ms
    report.glo_tow_acc = measx::TOW_ACC_OVER_4S;
    report.flags = 1U;        // towSet = 1

    rxm_measx_sv sv{};
    sv.gnss_id = 0U;
    sv.sv_id = 7U;
    sv.cno = 42U;
    sv.mpath_indic = static_cast<uint8_t>(rxm_measx_mpath::low);
    sv.doppler_ms = -250;     // -10 m/s
    sv.doppler_hz = 260;      // 52 Hz
    sv.whole_chips = 511U;
    sv.frac_chips = 512U;
    sv.code_phase = 1048576U; // 0.5 ms
    sv.int_code_phase = 3U;
    sv.pseu_range_rms_err = 20U;
    CASTLE_SAMPLE_CHECK(report.svs.push_back(sv) == castle::status::ok);

    uint8_t frame[UBX_FRAME_OVERHEAD + measx::header_length + measx::block_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(report, frame, sizeof(frame), written)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(measx::matches(raw));

    measx decoded{};
    CASTLE_SAMPLE_CHECK(measx::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.num_svs == 1U);
    CASTLE_SAMPLE_CHECK(decoded.svs.size() == 1U);
    CASTLE_SAMPLE_CHECK(decoded.gps_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(decoded.tow_set() == 1U);
    CASTLE_SAMPLE_CHECK(near(measx::tow_acc_ms(decoded.gps_tow_acc), 0.5, 1e-12));
    CASTLE_SAMPLE_CHECK(decoded.glo_tow_acc == measx::TOW_ACC_OVER_4S);

    const rxm_measx_sv& first = decoded.svs[0U];
    CASTLE_SAMPLE_CHECK(first.sv_id == 7U);
    CASTLE_SAMPLE_CHECK(first.mpath() == rxm_measx_mpath::low);
    CASTLE_SAMPLE_CHECK(near(first.doppler_speed_mps(), -10.0, 1e-12));
    CASTLE_SAMPLE_CHECK(near(first.doppler_frequency_hz(), 52.0, 1e-12));
    CASTLE_SAMPLE_CHECK(near(first.code_phase_ms(), 0.5, 1e-12));

    // A length that is not 44 + numSV*24 is rejected.
    message_view broken = raw;
    broken.header.payload_length = static_cast<uint16_t>(raw.payload_length() - 1U);
    CASTLE_SAMPLE_CHECK(!measx::decode(broken, decoded));

    return 0;
}
