#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/cfg_valset.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;
    using castle::protocols::ubx::config::config_entry;
    using castle::protocols::ubx::config::config_layer;
    using castle::protocols::ubx::config::config_value;

    static const uint32_t KEY_RATE_MEAS = 0x30210001U;  // CFG-RATE-MEAS, U2 [ms]
    static const uint32_t KEY_UART1_BAUD = 0x40520001U; // CFG-UART1-BAUDRATE, U4

    // 1. Version 0: write two items to RAM and BBR at once.
    cfg_valset<> set{};
    set.layers = config_layer::ram | config_layer::bbr;
    config_entry entry{};
    entry.key_id = KEY_RATE_MEAS;
    entry.value = config_value(static_cast<uint16_t>(200U));
    CASTLE_SAMPLE_CHECK(castle::succeeded(set.entries.push_back(entry)));
    entry.key_id = KEY_UART1_BAUD;
    entry.value = config_value(static_cast<uint32_t>(921600U));
    CASTLE_SAMPLE_CHECK(castle::succeeded(set.entries.push_back(entry)));
    CASTLE_SAMPLE_CHECK(set.ram() && set.bbr() && !set.flash());

    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(set, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + 4U + (4U + 2U) + (4U + 4U)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(cfg_valset<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!cfg_valset_transaction<>::matches(raw));

    cfg_valset<> echoed{};
    CASTLE_SAMPLE_CHECK(cfg_valset<>::decode(raw, echoed));
    CASTLE_SAMPLE_CHECK(echoed.layers == (cfg_valset<>::LAYERS_RAM | cfg_valset<>::LAYERS_BBR));
    CASTLE_SAMPLE_CHECK(echoed.entries.size() == 2U);
    CASTLE_SAMPLE_CHECK(echoed.entries[0U].value.as_u16() == 200U);
    CASTLE_SAMPLE_CHECK(echoed.entries[1U].value.as_u32() == 921600U);

    // 2. Version 1: the same change split over a transaction.
    cfg_valset_transaction<> begin{};
    begin.layers = cfg_valset_transaction<>::LAYERS_FLASH;
    begin.set_action(cfg_transaction_action::start);
    entry.key_id = KEY_RATE_MEAS;
    entry.value = config_value(static_cast<uint16_t>(200U));
    CASTLE_SAMPLE_CHECK(castle::succeeded(begin.entries.push_back(entry)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(begin, frame, sizeof(frame), written)));

    cfg_valset_transaction<> commit{};
    commit.layers = cfg_valset_transaction<>::LAYERS_FLASH;
    commit.set_action(cfg_transaction_action::apply); // no entries: only ends the transaction
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(commit, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + 4U));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));

    cfg_valset_transaction<> decoded{};
    CASTLE_SAMPLE_CHECK(cfg_valset_transaction<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.action() == cfg_transaction_action::apply);
    CASTLE_SAMPLE_CHECK(decoded.flash());
    CASTLE_SAMPLE_CHECK(decoded.entries.empty());

    // 3. A key without a known value width cannot be encoded.
    cfg_valset<> invalid{};
    entry.key_id = 0x00000001U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(invalid.entries.push_back(entry)));
    CASTLE_SAMPLE_CHECK(encode_message(invalid, frame, sizeof(frame), written) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(written == 0U);

    return 0;
}
