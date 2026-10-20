#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx_config.hpp"
#include "castle_ext/protocols/ubx/messages/cfg_valset.hpp"
#include "castle_ext/protocols/ubx/messages/cfg_valget.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::config;
    using castle::container::array_view;

    // ---------------------------------------------------------------------
    // config_value: strongly typed construction, raw-bit storage.
    // ---------------------------------------------------------------------
    config_value bool_value(true);
    config_value u32_value(static_cast<uint32_t>(123456U));
    config_value i32_value(static_cast<int32_t>(-42));

    CASTLE_SAMPLE_CHECK(bool_value.as_bool());
    CASTLE_SAMPLE_CHECK(u32_value.as_u32() == 123456U);
    CASTLE_SAMPLE_CHECK(i32_value.as_i32() == -42);
    CASTLE_SAMPLE_CHECK(i32_value.as_u32() == 0xFFFFFFD6U);
    CASTLE_SAMPLE_CHECK(u32_value != i32_value);

    // ---------------------------------------------------------------------
    // config_entry: the wire width follows the key's size code.
    // ---------------------------------------------------------------------
    CASTLE_CONST config_entry rate{0x30210001U, config_value(static_cast<uint16_t>(1000U))}; // CFG-RATE-MEAS (U2)
    CASTLE_CONST config_entry flag{0x10110002U, config_value(true)};                         // L (1 byte)
    CASTLE_SAMPLE_CHECK(value_byte_size(rate.key_id) == 2U);
    CASTLE_SAMPLE_CHECK(value_byte_size(flag.key_id) == 1U);

    // ---------------------------------------------------------------------
    // config_layer: VALSET/VALDEL bitmask; VALGET uses a selector instead.
    // ---------------------------------------------------------------------
    uint8_t layers = config_layer::ram | config_layer::flash;
    CASTLE_SAMPLE_CHECK(layers == 0x05U);
    CASTLE_SAMPLE_CHECK(UBX_CFG_VALGET_LAYER_FLASH == 2U);

    // ---------------------------------------------------------------------
    // The vocabulary feeds the CFG message models, which own the wire format.
    // ---------------------------------------------------------------------
    messages::cfg_valset<> set{};
    set.layers = layers;
    CASTLE_SAMPLE_CHECK(castle::succeeded(set.entries.push_back(rate)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(set.entries.push_back(flag)));

    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(set, frame, sizeof(frame), written)));

    message_view view;
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<const uint8_t>(frame, written), view));
    CASTLE_SAMPLE_CHECK(view.is(UBX_CLASS_CFG, UBX_ID_CFG_VALSET));

    messages::cfg_valset<> echoed{};
    CASTLE_SAMPLE_CHECK(messages::cfg_valset<>::decode(view, echoed));
    CASTLE_SAMPLE_CHECK(echoed.entries.size() == 2U);
    CASTLE_SAMPLE_CHECK(echoed.entries[0U].key_id == rate.key_id);
    CASTLE_SAMPLE_CHECK(echoed.entries[0U].value == rate.value);
    CASTLE_SAMPLE_CHECK(echoed.entries[1U].value.as_bool());

    // Poll the same keys back from RAM.
    messages::cfg_valget_poll<> poll{};
    poll.layer = UBX_CFG_VALGET_LAYER_RAM;
    CASTLE_SAMPLE_CHECK(castle::succeeded(poll.keys.push_back(rate.key_id)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(poll, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<const uint8_t>(frame, written), view));
    CASTLE_SAMPLE_CHECK(messages::cfg_valget_poll<>::matches(view));
    CASTLE_SAMPLE_CHECK(!messages::cfg_valget<>::matches(view)); // response layout is version 1

    return 0;
}
