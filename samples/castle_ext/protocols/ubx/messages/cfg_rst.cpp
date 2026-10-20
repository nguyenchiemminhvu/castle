#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/cfg_rst.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Cold start: clear every BBR section, then a controlled software reset.
    const cfg_rst cold = cfg_rst::cold_start();
    CASTLE_SAMPLE_CHECK(cold.nav_bbr_mask == cfg_rst::BBR_COLD_START);
    CASTLE_SAMPLE_CHECK(cold.reset_mode == cfg_rst_mode::software_reset);

    uint8_t frame[UBX_FRAME_OVERHEAD + cfg_rst::payload_length];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(cold, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == sizeof(frame));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(cfg_rst::matches(raw));

    cfg_rst decoded{};
    CASTLE_SAMPLE_CHECK(cfg_rst::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.nav_bbr_mask == cfg_rst::BBR_COLD_START);
    CASTLE_SAMPLE_CHECK(decoded.reset_mode == cfg_rst_mode::software_reset);

    // Hot / warm start factories with an explicit reset type.
    CASTLE_SAMPLE_CHECK(cfg_rst::hot_start().nav_bbr_mask == cfg_rst::BBR_HOT_START);
    const cfg_rst warm = cfg_rst::warm_start(cfg_rst_mode::hardware_reset);
    CASTLE_SAMPLE_CHECK(warm.nav_bbr_mask == cfg_rst::BBR_EPH);
    CASTLE_SAMPLE_CHECK(warm.reset_mode == cfg_rst_mode::hardware_reset);

    // Custom mask: drop almanac and position only, then restart GNSS.
    cfg_rst custom{};
    custom.nav_bbr_mask = cfg_rst::BBR_ALM | cfg_rst::BBR_POS;
    custom.reset_mode = cfg_rst_mode::gnss_start;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(custom, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(cfg_rst::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.nav_bbr_mask == 0x0012U);
    CASTLE_SAMPLE_CHECK(decoded.reset_mode == cfg_rst_mode::gnss_start);

    return 0;
}
