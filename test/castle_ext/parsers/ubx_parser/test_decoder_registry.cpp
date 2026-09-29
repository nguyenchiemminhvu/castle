#include <gtest/gtest.h>

#include "castle_ext/parsers/ubx_parser/decoder_registry.hpp"
#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"

namespace
{
using namespace castle_ext::parsers::ubx_parser;
using namespace castle_ext::protocols::ubx;

TEST(UbxDecoderRegistryMirror, FacadeAliasesAndAttachDispatchAck)
{
    using decoder = ack_ack_decoder<>;
    decoder_registry<decoder> registry;
    int hits = 0;
    auto connection = registry.decoder<castle_ext::protocols::ubx::messages::ack_ack>().connect(
        [&](const castle_ext::protocols::ubx::messages::ack_ack& value)
        {
            ++hits;
            EXPECT_EQ(value.cls_id, UBX_CLASS_CFG);
            EXPECT_EQ(value.msg_id_, UBX_ID_CFG_VALGET);
        });

    ubx_parser<> parser;
    castle_ext::parsers::ubx_parser::attach(parser, registry);

    uint8_t payload[2U] = {UBX_CLASS_CFG, UBX_ID_CFG_VALGET};
    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(UBX_CLASS_ACK, UBX_ID_ACK_ACK,
                           castle::container::array_view<CASTLE_CONST uint8_t>(payload, 2U),
                           frame, sizeof(frame), written), castle::status::ok);
    parser.feed(frame, written);
    EXPECT_EQ(hits, 1);
}

TEST(UbxDecoderRegistryMirror, DefaultRegistryCountMatchesBuiltInSet)
{
    EXPECT_EQ(default_decoder_registry<>::decoder_count(), 31U);
}

} // namespace
