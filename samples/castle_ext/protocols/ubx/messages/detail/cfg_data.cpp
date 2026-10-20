#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/cfg_valdel.hpp"
#include "castle_ext/protocols/ubx/messages/cfg_valset.hpp"
#include "castle_ext/protocols/ubx/messages/detail/cfg_data.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;
    using castle::container::array_view;

    static const uint32_t KEY_RATE_MEAS = 0x30210001U;  // U2
    static const uint32_t KEY_UART1_BAUD = 0x40520001U; // U4

    // 1. cfg_message through a real model: the base writes and checks the version byte.
    cfg_valdel_transaction<> del{};
    del.layers = cfg_valdel_transaction<>::LAYERS_FLASH;
    del.set_action(cfg_transaction_action::start);
    CASTLE_SAMPLE_CHECK(castle::succeeded(del.keys.push_back(KEY_RATE_MEAS)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(del.keys.push_back(KEY_UART1_BAUD)));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(del, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + cfg_valdel_transaction<>::header_length + (2U * 4U)));
    CASTLE_SAMPLE_CHECK(frame[UBX_FRAME_INDEX_PAYLOAD] == cfg_valdel_transaction<>::version);

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(cfg_valdel_transaction<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!cfg_valdel<>::matches(raw)); // same ID, version 0 layout

    // cfg_keys reads every U4 key back.
    cfg_valdel_transaction<> keys{};
    CASTLE_SAMPLE_CHECK(cfg_valdel_transaction<>::decode(raw, keys));
    CASTLE_SAMPLE_CHECK(keys.keys.size() == 2U);
    CASTLE_SAMPLE_CHECK(keys.keys[0U] == KEY_RATE_MEAS);
    CASTLE_SAMPLE_CHECK(keys.keys[1U] == KEY_UART1_BAUD);

    // 2. cfg_transaction_field: action() reads bits 1..0, set_action() keeps the other bits.
    CASTLE_SAMPLE_CHECK(keys.action() == cfg_transaction_action::start);
    keys.transaction = 0xF0U;
    keys.set_action(cfg_transaction_action::apply);
    CASTLE_SAMPLE_CHECK(keys.transaction == 0xF3U);
    CASTLE_SAMPLE_CHECK(keys.action() == cfg_transaction_action::apply);
    keys.set_action(cfg_transaction_action::ongoing);
    CASTLE_SAMPLE_CHECK(keys.action() == cfg_transaction_action::ongoing);
    keys.set_action(cfg_transaction_action::transactionless);
    CASTLE_SAMPLE_CHECK(keys.transaction == 0xF0U);

    // 3. cfg_data: each value is as wide as its key's size code.
    cfg_valset<> set{};
    set.layers = config::config_layer::ram | config::config_layer::bbr;
    config::config_entry entry{};
    entry.key_id = KEY_RATE_MEAS;
    entry.value = config::config_value(static_cast<uint16_t>(250U));
    CASTLE_SAMPLE_CHECK(castle::succeeded(set.entries.push_back(entry)));
    entry.key_id = KEY_UART1_BAUD;
    entry.value = config::config_value(static_cast<uint32_t>(460800U));
    CASTLE_SAMPLE_CHECK(castle::succeeded(set.entries.push_back(entry)));

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(set, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + 4U + (4U + 2U) + (4U + 4U)));
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<const uint8_t>(frame, written), raw));

    cfg_valset<> values{};
    CASTLE_SAMPLE_CHECK(cfg_valset<>::decode(raw, values));
    CASTLE_SAMPLE_CHECK(values.entries.size() == 2U);
    CASTLE_SAMPLE_CHECK(values.entries[0U].value.as_u16() == 250U);
    CASTLE_SAMPLE_CHECK(values.entries[1U].value.as_u32() == 460800U);

    // 4. A key with an unknown size code: encode latches invalid_argument, decode fails.
    cfg_valset<> invalid{};
    entry.key_id = 0x00000001U; // size code 0
    CASTLE_SAMPLE_CHECK(castle::succeeded(invalid.entries.push_back(entry)));
    CASTLE_SAMPLE_CHECK(encode_message(invalid, frame, sizeof(frame), written) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(written == 0U);

    uint8_t unknown[9U] = {cfg_valset<>::version, cfg_valset<>::LAYERS_RAM, 0U, 0U, 0x01U, 0x00U, 0x00U, 0x00U, 0x7FU};
    const message_view bad{{UBX_CLASS_CFG, UBX_ID_CFG_VALSET, sizeof(unknown)},
                           array_view<const uint8_t>(unknown, sizeof(unknown)), checksum{}};
    CASTLE_SAMPLE_CHECK(cfg_valset<>::matches(bad));
    CASTLE_SAMPLE_CHECK(!cfg_valset<>::decode(bad, values));
    CASTLE_SAMPLE_CHECK(values.entries.empty());

    // 5. Spec limit shared by every CFG-VAL* message.
    CASTLE_SAMPLE_CHECK(CFG_MAX_KEYS == 64U);
    CASTLE_SAMPLE_CHECK(cfg_valset<>().entries.capacity() == CFG_MAX_KEYS);

    return 0;
}
