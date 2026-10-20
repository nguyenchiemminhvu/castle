#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/cfg_valdel.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    static const uint32_t KEY_RATE_MEAS = 0x30210001U;  // CFG-RATE-MEAS
    static const uint32_t KEY_UART1_BAUD = 0x40520001U; // CFG-UART1-BAUDRATE

    // 1. Version 0: revert two items saved in Flash to their defaults.
    cfg_valdel<> del{};
    del.layers = cfg_valdel<>::LAYERS_FLASH;
    CASTLE_SAMPLE_CHECK(castle::succeeded(del.keys.push_back(KEY_RATE_MEAS)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(del.keys.push_back(KEY_UART1_BAUD)));
    CASTLE_SAMPLE_CHECK(del.flash() && !del.bbr());

    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(del, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + 4U + (2U * 4U)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(cfg_valdel<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!cfg_valdel_transaction<>::matches(raw));

    cfg_valdel<> echoed{};
    CASTLE_SAMPLE_CHECK(cfg_valdel<>::decode(raw, echoed));
    CASTLE_SAMPLE_CHECK(echoed.layers == cfg_valdel<>::LAYERS_FLASH);
    CASTLE_SAMPLE_CHECK(echoed.keys.size() == 2U);
    CASTLE_SAMPLE_CHECK(echoed.keys[0U] == KEY_RATE_MEAS);
    CASTLE_SAMPLE_CHECK(echoed.keys[1U] == KEY_UART1_BAUD);

    // 2. Version 1: delete from BBR and Flash inside a transaction.
    cfg_valdel_transaction<> tx{};
    tx.layers = cfg_valdel_transaction<>::LAYERS_BBR | cfg_valdel_transaction<>::LAYERS_FLASH;
    tx.set_action(cfg_transaction_action::ongoing);
    CASTLE_SAMPLE_CHECK(castle::succeeded(tx.keys.push_back(KEY_RATE_MEAS)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(tx, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(cfg_valdel_transaction<>::matches(raw));

    cfg_valdel_transaction<> decoded{};
    CASTLE_SAMPLE_CHECK(cfg_valdel_transaction<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.action() == cfg_transaction_action::ongoing);
    CASTLE_SAMPLE_CHECK(decoded.bbr() && decoded.flash());
    CASTLE_SAMPLE_CHECK(decoded.keys.size() == 1U);

    // A key list that is not a whole number of U4 values is rejected.
    message_view truncated = raw;
    truncated.header.payload_length = static_cast<uint16_t>(raw.payload_length() - 1U);
    truncated.payload = castle::container::array_view<const uint8_t>(raw.payload.data(), truncated.header.payload_length);
    CASTLE_SAMPLE_CHECK(!cfg_valdel_transaction<>::decode(truncated, decoded));

    return 0;
}
