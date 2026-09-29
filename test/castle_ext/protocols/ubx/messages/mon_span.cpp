#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_span.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageMonSpanMirror, DecodesOneSpectrumBlock)
{
    uint8_t payload[276U] = {};
    payload[0U] = 1U;
    payload[1U] = 1U;
    for (castle::size_type i = 0U; i < MON_SPAN_SPECTRUM_BINS; ++i)
    {
        payload[4U + i] = static_cast<uint8_t>(i);
    }
    castle::write_le32(&payload[260U], 0x01020304U);
    castle::write_le32(&payload[264U], 0x11121314U);
    castle::write_le32(&payload[268U], 0x21222324U);
    payload[272U] = 99U;

    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_SPAN, 276U}, array_view<CASTLE_CONST uint8_t>(payload, 276U), checksum{}};
    mon_span<2U> out;
    ASSERT_TRUE(mon_span<2U>::decode(raw, out));
    EXPECT_EQ(out.version, 1U);
    EXPECT_EQ(out.num_rf_blocks, 1U);
    EXPECT_EQ(out.rf_blocks[0U].spectrum[0U], 0U);
    EXPECT_EQ(out.rf_blocks[0U].spectrum[255U], 255U);
    EXPECT_EQ(out.rf_blocks[0U].span, 0x01020304U);
    EXPECT_EQ(out.rf_blocks[0U].pga, 99U);
}

TEST(UbxMessageMonSpanMirror, RejectsShortAndMisalignedPayloads)
{
    uint8_t payload[277U] = {};
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_SPAN, 3U}, array_view<CASTLE_CONST uint8_t>(payload, 277U), checksum{}};
    mon_span<> out;
    EXPECT_FALSE(mon_span<>::decode(raw, out));
    raw.header.payload_length = 277U;
    EXPECT_FALSE(mon_span<>::decode(raw, out));
}

} // namespace

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageMonSpanMirror, ClampsRfBlockStorageAndSkipsTrailingBlock)
{
    uint8_t payload[548U] = {};
    payload[0U] = 1U;
    payload[1U] = 2U;
    message_view raw{{UBX_CLASS_MON, UBX_ID_MON_SPAN, 548U},
                     array_view<CASTLE_CONST uint8_t>(payload, 548U), checksum{}};
    mon_span<1U> out;
    ASSERT_TRUE(mon_span<1U>::decode(raw, out));
    EXPECT_EQ(out.num_rf_blocks, 1U);
}

} // namespace
