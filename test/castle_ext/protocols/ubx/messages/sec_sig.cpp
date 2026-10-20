#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/sec_sig.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = sec_sig;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

message distinctive()
{
    message value{};
    value.version = 0x01U;
    value.jam_flags = 0xA5U;
    value.spf_flags = 0x5BU;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.version, 0U);
    EXPECT_EQ(value.jam_flags, 0U);
    EXPECT_EQ(value.spf_flags, 0U);
}

void fill_prefix(uint8_t* payload)
{
    for (castle::size_type i = 0U; i < message::payload_length; ++i)
    {
        payload[i] = 0xFFU; // reserved bytes are ignored on decode
    }
    payload[0U] = 0x01U;
    payload[4U] = 0x07U;
    payload[8U] = 0x0DU;
}

void expect_prefix(const message& value)
{
    EXPECT_EQ(value.version, 0x01U);
    EXPECT_EQ(value.jam_flags, 0x07U);
    EXPECT_EQ(value.spf_flags, 0x0DU);
}

TEST(UbxMessageSecSig, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_SEC);
    EXPECT_EQ(message::msg_id, UBX_ID_SEC_SIG);
    EXPECT_EQ(message::msg_class, 0x27U);
    EXPECT_EQ(message::msg_id, 0x09U);
    EXPECT_EQ(message::payload_length, 12U);
    EXPECT_EQ(message::JAM_FLAGS_JAM_DET_ENABLED, 0x01U);
    EXPECT_EQ(message::JAM_FLAGS_JAMMING_STATE, 0x06U);
    EXPECT_EQ(message::SPF_FLAGS_SPF_DET_ENABLED, 0x01U);
    EXPECT_EQ(message::SPF_FLAGS_SPOOFING_STATE, 0x0EU);
}

TEST(UbxMessageSecSig, EnumValuesMatchSpec)
{
    EXPECT_EQ(static_cast<uint8_t>(sec_sig_jamming_state::unknown), 0U);
    EXPECT_EQ(static_cast<uint8_t>(sec_sig_jamming_state::no_jamming), 1U);
    EXPECT_EQ(static_cast<uint8_t>(sec_sig_jamming_state::warning), 2U);
    EXPECT_EQ(static_cast<uint8_t>(sec_sig_jamming_state::critical), 3U);
    EXPECT_EQ(static_cast<uint8_t>(sec_sig_spoofing_state::unknown), 0U);
    EXPECT_EQ(static_cast<uint8_t>(sec_sig_spoofing_state::no_spoofing), 1U);
    EXPECT_EQ(static_cast<uint8_t>(sec_sig_spoofing_state::indicated), 2U);
    EXPECT_EQ(static_cast<uint8_t>(sec_sig_spoofing_state::affirmed), 3U);
}

TEST(UbxMessageSecSig, RoundTripPreservesEveryField)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(message::matches(raw));
    EXPECT_EQ(raw.payload_length(), message::payload_length);

    message decoded{};
    ASSERT_TRUE(message::decode(raw, decoded));
    EXPECT_EQ(decoded.version, original.version);
    EXPECT_EQ(decoded.jam_flags, original.jam_flags);
    EXPECT_EQ(decoded.spf_flags, original.spf_flags);
}

TEST(UbxMessageSecSig, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    fill_prefix(payload);

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    expect_prefix(decoded);
    EXPECT_TRUE(decoded.jam_det_enabled());
    EXPECT_EQ(decoded.jamming_state(), sec_sig_jamming_state::critical);
    EXPECT_TRUE(decoded.spf_det_enabled());
    EXPECT_EQ(decoded.spoofing_state(), static_cast<sec_sig_spoofing_state>(6U));
}

TEST(UbxMessageSecSig, EncodesFieldsAtSpecOffsetsWithZeroReserved)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length + 4U];
    for (castle::size_type i = 0U; i < sizeof(frame); ++i)
    {
        frame[i] = 0xEEU;
    }
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD + 12U);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_SEC);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_SEC_SIG);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 12U);
    EXPECT_EQ(payload[0U], 0x01U);
    EXPECT_EQ(payload[1U], 0U);
    EXPECT_EQ(payload[2U], 0U);
    EXPECT_EQ(payload[3U], 0U);
    EXPECT_EQ(payload[4U], 0xA5U);
    EXPECT_EQ(payload[5U], 0U);
    EXPECT_EQ(payload[6U], 0U);
    EXPECT_EQ(payload[7U], 0U);
    EXPECT_EQ(payload[8U], 0x5BU);
    EXPECT_EQ(payload[9U], 0U);
    EXPECT_EQ(payload[10U], 0U);
    EXPECT_EQ(payload[11U], 0U);
    for (castle::size_type i = written; i < sizeof(frame); ++i)
    {
        EXPECT_EQ(frame[i], 0xEEU);
    }
}

TEST(UbxMessageSecSig, AcceptsLongerPayloadsAndIgnoresTail)
{
    uint8_t payload[24U] = {};
    fill_prefix(payload);
    for (castle::size_type i = message::payload_length; i < sizeof(payload); ++i)
    {
        payload[i] = static_cast<uint8_t>(0x30U + i);
    }

    const castle::size_type lengths[] = {12U, 13U, 24U};
    for (castle::size_type length : lengths)
    {
        message decoded = distinctive();
        EXPECT_TRUE(message::matches(view_of(payload, length)));
        ASSERT_TRUE(message::decode(view_of(payload, length), decoded)) << "length " << length;
        expect_prefix(decoded);
    }
}

TEST(UbxMessageSecSig, RejectsWrongClassOrIdEvenWithLongPayload)
{
    uint8_t payload[24U] = {};
    fill_prefix(payload);
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_TIM;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_SEC_CRC;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageSecSig, RejectsShortPayloads)
{
    uint8_t payload[message::payload_length] = {};
    fill_prefix(payload);

    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, 0U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageSecSig, JamDetEnabledSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.jam_det_enabled());
    value.jam_flags = message::JAM_FLAGS_JAM_DET_ENABLED;
    EXPECT_TRUE(value.jam_det_enabled());
    EXPECT_EQ(value.jamming_state(), sec_sig_jamming_state::unknown);
    value.jam_flags = static_cast<uint8_t>(~message::JAM_FLAGS_JAM_DET_ENABLED);
    EXPECT_FALSE(value.jam_det_enabled());
    EXPECT_EQ(value.jamming_state(), sec_sig_jamming_state::critical);
}

TEST(UbxMessageSecSig, JammingStateAllValues)
{
    message value{};
    value.jam_flags = 0x00U;
    EXPECT_EQ(value.jamming_state(), sec_sig_jamming_state::unknown);
    value.jam_flags = 0x02U;
    EXPECT_EQ(value.jamming_state(), sec_sig_jamming_state::no_jamming);
    value.jam_flags = 0x04U;
    EXPECT_EQ(value.jamming_state(), sec_sig_jamming_state::warning);
    value.jam_flags = 0x06U;
    EXPECT_EQ(value.jamming_state(), sec_sig_jamming_state::critical);

    // Neighbouring bits (enable bit 0, bits 7..3) do not leak into the state.
    value.jam_flags = static_cast<uint8_t>(0xF9U | 0x02U);
    EXPECT_EQ(value.jamming_state(), sec_sig_jamming_state::no_jamming);
    EXPECT_TRUE(value.jam_det_enabled());
    value.jam_flags = 0xF9U;
    EXPECT_EQ(value.jamming_state(), sec_sig_jamming_state::unknown);
}

TEST(UbxMessageSecSig, SpfDetEnabledSetAndClear)
{
    message value{};
    EXPECT_FALSE(value.spf_det_enabled());
    value.spf_flags = message::SPF_FLAGS_SPF_DET_ENABLED;
    EXPECT_TRUE(value.spf_det_enabled());
    EXPECT_EQ(value.spoofing_state(), sec_sig_spoofing_state::unknown);
    value.spf_flags = static_cast<uint8_t>(~message::SPF_FLAGS_SPF_DET_ENABLED);
    EXPECT_FALSE(value.spf_det_enabled());
    EXPECT_EQ(static_cast<uint8_t>(value.spoofing_state()), 7U);
}

TEST(UbxMessageSecSig, SpoofingStateAllValuesAndRawPassThrough)
{
    message value{};
    value.spf_flags = 0x00U;
    EXPECT_EQ(value.spoofing_state(), sec_sig_spoofing_state::unknown);
    value.spf_flags = 0x02U;
    EXPECT_EQ(value.spoofing_state(), sec_sig_spoofing_state::no_spoofing);
    value.spf_flags = 0x04U;
    EXPECT_EQ(value.spoofing_state(), sec_sig_spoofing_state::indicated);
    value.spf_flags = 0x06U;
    EXPECT_EQ(value.spoofing_state(), sec_sig_spoofing_state::affirmed);

    // Raw 3-bit values without an enumerator are passed through unchanged.
    value.spf_flags = 0x0AU;
    EXPECT_EQ(static_cast<uint8_t>(value.spoofing_state()), 5U);
    value.spf_flags = 0x0CU;
    EXPECT_EQ(static_cast<uint8_t>(value.spoofing_state()), 6U);
    value.spf_flags = 0x0EU;
    EXPECT_EQ(static_cast<uint8_t>(value.spoofing_state()), 7U);

    // Neighbouring bits (enable bit 0, bits 7..4) do not leak into the state.
    value.spf_flags = 0xF3U;
    EXPECT_EQ(value.spoofing_state(), sec_sig_spoofing_state::no_spoofing);
    EXPECT_TRUE(value.spf_det_enabled());
    value.spf_flags = 0xF1U;
    EXPECT_EQ(value.spoofing_state(), sec_sig_spoofing_state::unknown);
}

TEST(UbxMessageSecSig, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
