#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/cfg_valget.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    static const uint32_t KEY_RATE_MEAS = 0x30210001U;   // CFG-RATE-MEAS, U2
    static const uint32_t KEY_UART1_BAUD = 0x40520001U;  // CFG-UART1-BAUDRATE, U4
    static const uint32_t KEY_NAVSPG_FIX = 0x20110011U;  // CFG-NAVSPG-FIXMODE, E1

    // 1. Host: poll three items from the RAM layer.
    cfg_valget_poll<> poll{};
    poll.layer = config::UBX_CFG_VALGET_LAYER_RAM;
    poll.position = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(poll.keys.push_back(KEY_RATE_MEAS)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(poll.keys.push_back(KEY_UART1_BAUD)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(poll.keys.push_back(KEY_NAVSPG_FIX)));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(poll, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + 4U + (3U * 4U)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(cfg_valget_poll<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!cfg_valget<>::matches(raw)); // version 0 is the request

    cfg_valget_poll<> echoed{};
    CASTLE_SAMPLE_CHECK(cfg_valget_poll<>::decode(raw, echoed));
    CASTLE_SAMPLE_CHECK(echoed.keys.size() == 3U);
    CASTLE_SAMPLE_CHECK(echoed.keys[1U] == KEY_UART1_BAUD);

    // 2. Receiver: the version 1 response with one value per key.
    cfg_valget<> response{};
    response.layer = config::UBX_CFG_VALGET_LAYER_RAM;
    cfg_valget_entry entry{};
    entry.key_id = KEY_RATE_MEAS;
    entry.value = 100U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(response.entries.push_back(entry)));
    entry.key_id = KEY_UART1_BAUD;
    entry.value = 115200U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(response.entries.push_back(entry)));
    entry.key_id = KEY_NAVSPG_FIX;
    entry.value = 3U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(response.entries.push_back(entry)));

    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(response, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + 4U + (4U + 2U) + (4U + 4U) + (4U + 1U)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(cfg_valget<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!cfg_valget_poll<>::matches(raw));

    cfg_valget<> decoded{};
    CASTLE_SAMPLE_CHECK(cfg_valget<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == cfg_valget<>::version);
    CASTLE_SAMPLE_CHECK(decoded.layer == config::UBX_CFG_VALGET_LAYER_RAM);
    CASTLE_SAMPLE_CHECK(decoded.position == 0U);
    CASTLE_SAMPLE_CHECK(decoded.entries.size() == 3U);
    CASTLE_SAMPLE_CHECK(decoded.entries[0U].key_id == KEY_RATE_MEAS);
    CASTLE_SAMPLE_CHECK(decoded.entries[0U].value == 100U);
    CASTLE_SAMPLE_CHECK(decoded.entries[1U].value == 115200U);
    CASTLE_SAMPLE_CHECK(decoded.entries[2U].value == 3U);

    // 3. A response with a key of unknown size cannot be walked and is rejected.
    uint8_t bad[8U] = {cfg_valget<>::version, 0U, 0U, 0U, 0x01U, 0x00U, 0x00U, 0x00U};
    message_view malformed{{UBX_CLASS_CFG, UBX_ID_CFG_VALGET, 8U},
                           castle::container::array_view<const uint8_t>(bad, 8U), checksum{}};
    CASTLE_SAMPLE_CHECK(!cfg_valget<>::decode(malformed, decoded));
    CASTLE_SAMPLE_CHECK(decoded.entries.empty());

    return 0;
}
