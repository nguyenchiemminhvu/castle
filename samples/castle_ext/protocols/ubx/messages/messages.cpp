#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/messages.hpp"

// The umbrella header brings in every UBX message model. This sample walks a
// typical host session using models from several classes: configure, reset,
// poll, then decode receiver outputs from the same frame buffer.

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;
    using castle::container::array_view;

    uint8_t frame[256U] = {};
    castle::size_type written = 0U;
    message_view raw;

    // 1. Configure: write CFG-RATE-MEAS = 100 ms into RAM.
    cfg_valset<> set{};
    set.layers = cfg_valset<>::LAYERS_RAM;
    CASTLE_SAMPLE_CHECK(castle::succeeded(set.entries.push_back(
        config::config_entry{0x30210001U, config::config_value(static_cast<uint16_t>(100U))})));
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(set, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(cfg_valset<>::matches(raw));

    // 2. The receiver acknowledges it.
    ack_ack ack{};
    ack.cls_id = UBX_CLASS_CFG;
    ack.acked_msg_id = UBX_ID_CFG_VALSET;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(ack, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<const uint8_t>(frame, written), raw));
    ack_ack received_ack{};
    CASTLE_SAMPLE_CHECK(ack_ack::decode(raw, received_ack));
    CASTLE_SAMPLE_CHECK(received_ack.acked_msg_id == UBX_ID_CFG_VALSET);
    CASTLE_SAMPLE_CHECK(!ack_nak::matches(raw));

    // 3. Reset (cold start) and poll the firmware version.
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(cfg_rst::cold_start(), frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_poll<mon_ver<>>(frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == UBX_FRAME_OVERHEAD);

    // 4. Assistance: initial position (MGA-INI-POS_LLH).
    mga_ini_pos_llh position{};
    position.lat = 473977418;
    position.lon = 85455939;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(position, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mga_ini_pos_llh::matches(raw));
    CASTLE_SAMPLE_CHECK(!mga_ini_pos_xyz::matches(raw)); // same class/ID, different type byte

    // 5. Receiver outputs: a high-precision position and the end-of-epoch marker.
    nav_hpposllh hp{};
    hp.i_tow = 345600000U;
    hp.lat = 473977418;
    hp.lat_hp = 25;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(hp, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<const uint8_t>(frame, written), raw));
    nav_hpposllh decoded_hp{};
    CASTLE_SAMPLE_CHECK(nav_hpposllh::decode(raw, decoded_hp));
    CASTLE_SAMPLE_CHECK(decoded_hp.lat_hp == 25);

    nav_eoe eoe{};
    eoe.i_tow = hp.i_tow;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(eoe, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<const uint8_t>(frame, written), raw));
    nav_eoe decoded_eoe{};
    CASTLE_SAMPLE_CHECK(nav_eoe::decode(raw, decoded_eoe));
    CASTLE_SAMPLE_CHECK(decoded_eoe.i_tow == decoded_hp.i_tow);

    // A decoder never accepts another message's frame.
    CASTLE_SAMPLE_CHECK(!nav_pvt::matches(raw));
    CASTLE_SAMPLE_CHECK(!nav2_pvt::matches(raw));

    return 0;
}
