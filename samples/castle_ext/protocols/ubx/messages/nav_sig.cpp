#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/nav_sig.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Build a NAV-SIG value with two signals.
    nav_sig<4U> sig{};
    sig.i_tow = 654321U;
    sig.version = 0U;

    nav_sig_info l1{};
    l1.gnss_id = 0U; // GPS
    l1.sv_id = 5U;
    l1.sig_id = 0U; // L1C/A
    l1.pr_res = 12; // 1.2 m
    l1.cno = 45U;
    l1.qual_ind = nav_signal_quality::code_carrier_locked;
    l1.corr_source = nav_sig_corr_source::sbas;
    l1.iono_model = nav_sig_iono_model::sbas;
    l1.sig_flags = 1U /* healthy */ | nav_sig_info::SIG_FLAGS_PR_USED | nav_sig_info::SIG_FLAGS_PR_CORR_USED;
    CASTLE_SAMPLE_CHECK(castle::succeeded(sig.signals.push_back(l1)));

    nav_sig_info e5b{};
    e5b.gnss_id = 2U; // Galileo
    e5b.sv_id = 12U;
    e5b.sig_id = 5U; // E5b I
    e5b.pr_res = -8;
    e5b.cno = 50U;
    e5b.qual_ind = nav_signal_quality::code_locked;
    e5b.iono_model = nav_sig_iono_model::dual_frequency;
    e5b.sig_flags = 2U /* unhealthy */ | nav_sig_info::SIG_FLAGS_PR_SMOOTHED;
    CASTLE_SAMPLE_CHECK(castle::succeeded(sig.signals.push_back(e5b)));

    // Encode into a complete UBX frame; numSigs comes from signals.size().
    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(sig, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + nav_sig<>::header_length + (2U * nav_sig<>::block_length)));

    // Parse the frame back.
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(nav_sig<>::matches(raw));

    nav_sig<> decoded;
    CASTLE_SAMPLE_CHECK(nav_sig<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.i_tow == 654321U);
    CASTLE_SAMPLE_CHECK(decoded.num_sigs == 2U);
    CASTLE_SAMPLE_CHECK(decoded.signals.size() == 2U);

    CASTLE_SAMPLE_CHECK(decoded.signals[0U].sv_id == 5U);
    CASTLE_SAMPLE_CHECK(decoded.signals[0U].pr_res_m() > 1.19);
    CASTLE_SAMPLE_CHECK(decoded.signals[0U].pr_res_m() < 1.21);
    CASTLE_SAMPLE_CHECK(decoded.signals[0U].qual_ind == nav_signal_quality::code_carrier_locked);
    CASTLE_SAMPLE_CHECK(decoded.signals[0U].corr_source == nav_sig_corr_source::sbas);
    CASTLE_SAMPLE_CHECK(decoded.signals[0U].health() == nav_sv_health::healthy);
    CASTLE_SAMPLE_CHECK(decoded.signals[0U].pr_used());
    CASTLE_SAMPLE_CHECK(decoded.signals[0U].pr_corr_used());
    CASTLE_SAMPLE_CHECK(!decoded.signals[0U].cr_used());

    CASTLE_SAMPLE_CHECK(decoded.signals[1U].sig_id == 5U);
    CASTLE_SAMPLE_CHECK(decoded.signals[1U].iono_model == nav_sig_iono_model::dual_frequency);
    CASTLE_SAMPLE_CHECK(decoded.signals[1U].health() == nav_sv_health::unhealthy);
    CASTLE_SAMPLE_CHECK(decoded.signals[1U].pr_smoothed());
    CASTLE_SAMPLE_CHECK(!decoded.signals[1U].pr_used());

    // A smaller capacity keeps the first block and still reports the wire count.
    nav_sig<1U> clipped;
    CASTLE_SAMPLE_CHECK(nav_sig<1U>::decode(raw, clipped));
    CASTLE_SAMPLE_CHECK(clipped.num_sigs == 2U);
    CASTLE_SAMPLE_CHECK(clipped.signals.size() == 1U);

    // A different NAV message ID does not match NAV-SIG.
    raw.header.msg_id = UBX_ID_NAV_PVT;
    CASTLE_SAMPLE_CHECK(!nav_sig<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!nav_sig<>::decode(raw, decoded));

    return 0;
}
