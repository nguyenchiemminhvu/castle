#include <gtest/gtest.h>

#include "castle_ext/parsers/ubx_parser/decoder_registry.hpp"
#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"

namespace
{
using namespace castle::parsers::ubx_parser;
using namespace castle::protocols::ubx;

TEST(UbxDecoderRegistryMirror, FacadeAliasesAndAttachDispatchAck)
{
    using decoder = ack_ack_decoder<>;
    decoder_registry<decoder> registry;
    int hits = 0;
    auto connection = registry.decoder<castle::protocols::ubx::messages::ack_ack>().connect(
        [&](const castle::protocols::ubx::messages::ack_ack& value)
        {
            ++hits;
            EXPECT_EQ(value.cls_id, UBX_CLASS_CFG);
            EXPECT_EQ(value.acked_msg_id, UBX_ID_CFG_VALGET);
        });

    ubx_parser<> parser;
    castle::parsers::ubx_parser::attach(parser, registry);

    uint8_t payload[2U] = {UBX_CLASS_CFG, UBX_ID_CFG_VALGET};
    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(UBX_CLASS_ACK, UBX_ID_ACK_ACK,
                           castle::container::array_view<CASTLE_CONST uint8_t>(payload, 2U),
                           frame, sizeof(frame), written), castle::status::ok);
    parser.feed(frame, written);
    EXPECT_EQ(hits, 1);
}

template <typename Message>
castle::parsers::ubx_parser::dispatch_result dispatch_encoded(
    default_decoder_registry<>& registry, CASTLE_CONST Message& message)
{
    static uint8_t frame[UBX_SAFE_MAX_PAYLOAD_LEN + UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    if (encode_message(message, frame, sizeof(frame), written) != castle::status::ok)
    {
        return {};
    }
    message_view raw;
    if (!decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), raw))
    {
        return {};
    }
    return registry.dispatch(raw);
}

TEST(UbxDecoderRegistryMirror, DefaultRegistryDispatchesEachOutputToExactlyOneDecoder)
{
    using namespace castle::protocols::ubx::messages;
    static default_decoder_registry<> registry;

    // One representative per layout family: fixed, typed, repeated-group,
    // NAV vs NAV2 twins, and the two UPD-SOS outputs sharing one class/ID.
    CASTLE_CONST castle::parsers::ubx_parser::dispatch_result results[] = {
        dispatch_encoded(registry, nav_pvt{}),
        dispatch_encoded(registry, nav2_pvt{}),
        dispatch_encoded(registry, nav_relposned{}),
        dispatch_encoded(registry, nav_sat<>{}),
        dispatch_encoded(registry, nav_timebds{}),
        dispatch_encoded(registry, mon_ver<>{}),
        dispatch_encoded(registry, mon_rf<>{}),
        dispatch_encoded(registry, esf_status<>{}),
        dispatch_encoded(registry, rxm_rawx<>{}),
        dispatch_encoded(registry, rxm_rlm_short{}),
        dispatch_encoded(registry, rxm_rlm_long{}),
        dispatch_encoded(registry, mga_ack{}),
        dispatch_encoded(registry, tim_tm2{}),
        dispatch_encoded(registry, sec_uniqid{}),
        dispatch_encoded(registry, upd_sos_ack{}),
        dispatch_encoded(registry, upd_sos_restored{}),
    };

    for (CASTLE_CONST auto& result : results)
    {
        EXPECT_EQ(result.matched, 1U);
        EXPECT_EQ(result.decoded, 1U);
    }
}

TEST(UbxDecoderRegistryMirror, DefaultRegistryIgnoresHostCommands)
{
    using namespace castle::protocols::ubx::messages;
    static default_decoder_registry<> registry;

    EXPECT_EQ(dispatch_encoded(registry, cfg_rst::cold_start()).matched, 0U);
    EXPECT_EQ(dispatch_encoded(registry, mga_gps_utc{}).matched, 0U);
    EXPECT_EQ(dispatch_encoded(registry, upd_sos_save{}).matched, 0U);
}

} // namespace
