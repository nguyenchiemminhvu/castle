#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/ubx_config.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using namespace castle_ext::protocols::ubx::config;

    // ---------------------------------------------------------------------
    // Build a UBX-CFG-VALSET request that writes two configuration keys.
    // ---------------------------------------------------------------------

    config_entry valset_entries[2U] =
    {
        // Example: CFG-RATE-MEAS (U2)
        {0x30210001U, config_value(static_cast<uint16_t>(1000U))},

        // Example: boolean configuration item (L/U1-sized key)
        {0x10110002U, config_value(true)}
    };

    uint8_t valset_frame[128U];
    castle::size_type valset_written = 0U;

    castle::status valset_status =
        build_valset_frame(
            castle::container::array_view<const config_entry>(valset_entries, 2U),
            static_cast<uint8_t>(config_layer::ram | config_layer::flash),
            valset_frame,
            sizeof(valset_frame),
            valset_written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(valset_status));
    CASTLE_SAMPLE_CHECK(valset_written > 0U);

    // Decode the generated UBX frame.
    message_view valset_view;
    bool valset_decoded =
        decode_frame(
            castle::container::array_view<const uint8_t>(
                valset_frame,
                valset_written),
            valset_view);

    CASTLE_SAMPLE_CHECK(valset_decoded);
    CASTLE_SAMPLE_CHECK(valset_view.is(UBX_CLASS_CFG, UBX_ID_CFG_VALSET));
    CASTLE_SAMPLE_CHECK(valset_view.payload_length() > 4U);

    // ---------------------------------------------------------------------
    // Build a UBX-CFG-VALGET poll request.
    // ---------------------------------------------------------------------

    uint32_t keys[2U] =
    {
        0x30210001U,
        0x10110002U
    };

    uint8_t valget_frame[128U];
    castle::size_type valget_written = 0U;

    castle::status valget_status =
        build_valget_frame(
            castle::container::array_view<const uint32_t>(keys, 2U),
            UBX_CFG_VALGET_LAYER_RAM,
            0U,
            valget_frame,
            sizeof(valget_frame),
            valget_written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(valget_status));

    message_view valget_view;
    bool valget_decoded =
        decode_frame(
            castle::container::array_view<const uint8_t>(
                valget_frame,
                valget_written),
            valget_view);

    CASTLE_SAMPLE_CHECK(valget_decoded);
    CASTLE_SAMPLE_CHECK(valget_view.is(UBX_CLASS_CFG, UBX_ID_CFG_VALGET));

    // ---------------------------------------------------------------------
    // Construct a synthetic UBX-CFG-VALGET response payload and parse it.
    //
    // Response format:
    // [version][layer][position_lo][position_hi]
    // [key0][value0]
    // [key1][value1]
    // ---------------------------------------------------------------------

    uint8_t response_payload[32U];
    castle::size_type pos = 0U;

    response_payload[pos++] = UBX_VALGET_VERSION_RESP;
    response_payload[pos++] = UBX_CFG_VALGET_LAYER_RAM;
    response_payload[pos++] = 0U;
    response_payload[pos++] = 0U;

    // Key: 0x30210001 (U2)
    write_le32(&response_payload[pos], 0x30210001U);
    pos += 4U;
    write_le16(&response_payload[pos], 1000U);
    pos += 2U;

    // Key: 0x10110002 (U1)
    write_le32(&response_payload[pos], 0x10110002U);
    pos += 4U;
    response_payload[pos++] = 1U;

    config_entry parsed_entries[4U];

    castle::size_type parsed_count =
        parse_valget_response(
            castle::container::array_view<const uint8_t>(
                response_payload,
                pos),
            parsed_entries,
            4U);

    CASTLE_SAMPLE_CHECK(parsed_count == 2U);

    CASTLE_SAMPLE_CHECK(parsed_entries[0U].key_id == 0x30210001U);
    CASTLE_SAMPLE_CHECK(parsed_entries[0U].value.as_u16() == 1000U);

    CASTLE_SAMPLE_CHECK(parsed_entries[1U].key_id == 0x10110002U);
    CASTLE_SAMPLE_CHECK(parsed_entries[1U].value.as_bool());

    // ---------------------------------------------------------------------
    // config_value supports strongly typed construction and extraction.
    // ---------------------------------------------------------------------

    config_value bool_value(true);
    config_value u32_value(static_cast<uint32_t>(123456U));
    config_value i32_value(static_cast<int32_t>(-42));

    CASTLE_SAMPLE_CHECK(bool_value.as_bool());
    CASTLE_SAMPLE_CHECK(u32_value.as_u32() == 123456U);
    CASTLE_SAMPLE_CHECK(i32_value.as_i32() == -42);

    // ---------------------------------------------------------------------
    // Layer flag composition helper.
    // ---------------------------------------------------------------------

    uint8_t layers =
        config_layer::ram
        | config_layer::bbr;

    CASTLE_SAMPLE_CHECK(layers == 0x03U);

    // ---------------------------------------------------------------------
    // Invalid input checks.
    // ---------------------------------------------------------------------

    castle::size_type rejected_written = 0U;

    CASTLE_SAMPLE_CHECK(
        build_valset_frame(
            castle::container::array_view<const config_entry>(),
            static_cast<uint8_t>(config_layer::ram),
            valset_frame,
            sizeof(valset_frame),
            rejected_written)
        == castle::status::invalid_argument);

    CASTLE_SAMPLE_CHECK(
        build_valget_frame(
            castle::container::array_view<const uint32_t>(),
            UBX_CFG_VALGET_LAYER_RAM,
            0U,
            valget_frame,
            sizeof(valget_frame),
            rejected_written)
        == castle::status::invalid_argument);

    return 0;
}
