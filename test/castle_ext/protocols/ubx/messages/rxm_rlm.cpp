#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/rxm_rlm.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(const uint8_t* payload,
                     castle::size_type length,
                     uint8_t msg_class = UBX_CLASS_RXM,
                     uint8_t msg_id = UBX_ID_RXM_RLM)
{
    return message_view{{msg_class, msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

template <typename Message>
Message distinctive()
{
    Message value{};
    value.sv_id = 0x1DU;
    for (castle::size_type index = 0U; index < value.beacon.size(); ++index)
    {
        value.beacon[index] = static_cast<uint8_t>(0x31U + (index * 0x11U));
    }
    value.message = 0x0BU;
    for (castle::size_type index = 0U; index < value.params.size(); ++index)
    {
        value.params[index] = static_cast<uint8_t>(0xC1U + (index * 0x03U));
    }
    return value;
}

template <typename Message>
void expect_default(const Message& value)
{
    EXPECT_EQ(value.sv_id, 0U);
    for (const uint8_t byte : value.beacon)
    {
        EXPECT_EQ(byte, 0U);
    }
    EXPECT_EQ(value.message, 0U);
    for (const uint8_t byte : value.params)
    {
        EXPECT_EQ(byte, 0U);
    }
    EXPECT_EQ(value.beacon_id(), 0U);
}

/** @brief Fills a valid `Message` payload with known contents at the spec offsets. */
template <typename Message>
void fill_payload(uint8_t* payload)
{
    payload[0U] = RXM_RLM_VERSION;
    payload[1U] = Message::message_type;
    payload[2U] = 0x24U; // svId
    payload[3U] = 0xEEU; // reserved0, ignored on decode
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        payload[4U + index] = static_cast<uint8_t>(0x01U + index); // beacon
    }
    payload[12U] = 0x09U; // message
    const castle::size_type param_bytes = Message{}.params.size();
    for (castle::size_type index = 0U; index < param_bytes; ++index)
    {
        payload[13U + index] = static_cast<uint8_t>(0xA0U + index); // params
    }
    for (castle::size_type index = 13U + param_bytes; index < Message::payload_length; ++index)
    {
        payload[index] = 0xDDU; // reserved1, ignored on decode
    }
}

template <typename Message>
void expect_spec_payload(const Message& decoded)
{
    EXPECT_EQ(decoded.sv_id, 0x24U);
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        EXPECT_EQ(decoded.beacon[index], 0x01U + index) << "beacon[" << index << "]";
    }
    EXPECT_EQ(decoded.message, 0x09U);
    for (castle::size_type index = 0U; index < decoded.params.size(); ++index)
    {
        EXPECT_EQ(decoded.params[index], 0xA0U + index) << "params[" << index << "]";
    }
    EXPECT_EQ(decoded.beacon_id(), 0x0102030405060708ULL & 0x0FFFFFFFFFFFFFFFULL);
}

template <typename Message>
void expect_round_trip()
{
    const Message original = distinctive<Message>();
    uint8_t frame[UBX_FRAME_OVERHEAD + Message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(Message::matches(raw));
    EXPECT_EQ(raw.payload_length(), Message::payload_length);

    Message decoded{};
    ASSERT_TRUE(Message::decode(raw, decoded));
    EXPECT_EQ(decoded.sv_id, original.sv_id);
    for (castle::size_type index = 0U; index < original.beacon.size(); ++index)
    {
        EXPECT_EQ(decoded.beacon[index], original.beacon[index]) << "beacon[" << index << "]";
    }
    EXPECT_EQ(decoded.message, original.message);
    for (castle::size_type index = 0U; index < original.params.size(); ++index)
    {
        EXPECT_EQ(decoded.params[index], original.params[index]) << "params[" << index << "]";
    }
    EXPECT_EQ(decoded.beacon_id(), original.beacon_id());
}

template <typename Message>
void expect_encoded_at_spec_offsets()
{
    const Message original = distinctive<Message>();
    uint8_t frame[UBX_FRAME_OVERHEAD + Message::payload_length];
    for (uint8_t& byte : frame)
    {
        byte = 0x5AU; // poison so reserved bytes must be actively zeroed
    }
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_RXM);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_RXM_RLM);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), Message::payload_length);
    EXPECT_EQ(payload[0U], RXM_RLM_VERSION);
    EXPECT_EQ(payload[1U], Message::message_type);
    EXPECT_EQ(payload[2U], 0x1DU);
    EXPECT_EQ(payload[3U], 0x00U);
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        EXPECT_EQ(payload[4U + index], 0x31U + (index * 0x11U)) << "beacon[" << index << "]";
    }
    EXPECT_EQ(payload[12U], 0x0BU);
    const castle::size_type param_bytes = original.params.size();
    for (castle::size_type index = 0U; index < param_bytes; ++index)
    {
        EXPECT_EQ(payload[13U + index], 0xC1U + (index * 0x03U)) << "params[" << index << "]";
    }
    for (castle::size_type index = 13U + param_bytes; index < Message::payload_length; ++index)
    {
        EXPECT_EQ(payload[index], 0x00U) << "reserved1 at " << index;
    }
}

template <typename Message>
void expect_rejects_wrong_class_or_id()
{
    uint8_t payload[Message::payload_length] = {};
    fill_payload<Message>(payload);

    Message decoded = distinctive<Message>();
    message_view raw = view_of(payload, sizeof(payload), UBX_CLASS_NAV, UBX_ID_RXM_RLM);
    EXPECT_FALSE(Message::matches(raw));
    EXPECT_FALSE(Message::decode(raw, decoded));
    expect_default(decoded);

    raw = view_of(payload, sizeof(payload), UBX_CLASS_RXM, UBX_ID_RXM_RTCM);
    EXPECT_FALSE(Message::matches(raw));
    decoded = distinctive<Message>();
    EXPECT_FALSE(Message::decode(raw, decoded));
    expect_default(decoded);
}

template <typename Message>
void expect_rejects_short_and_long()
{
    uint8_t payload[rxm_rlm_long::payload_length + 1U] = {};
    fill_payload<Message>(payload);

    Message decoded = distinctive<Message>();
    EXPECT_FALSE(Message::matches(view_of(payload, Message::payload_length - 1U)));
    EXPECT_FALSE(Message::decode(view_of(payload, Message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive<Message>();
    EXPECT_FALSE(Message::matches(view_of(payload, Message::payload_length + 1U)));
    EXPECT_FALSE(Message::decode(view_of(payload, Message::payload_length + 1U), decoded));
    expect_default(decoded);

    decoded = distinctive<Message>();
    EXPECT_FALSE(Message::matches(view_of(payload, 0U)));
    EXPECT_FALSE(Message::decode(view_of(payload, 0U), decoded));
    expect_default(decoded);
}

template <typename Message>
void expect_rejects_wrong_version()
{
    uint8_t payload[Message::payload_length] = {};
    fill_payload<Message>(payload);

    const uint8_t versions[] = {0x01U, 0x02U, 0xFFU};
    for (const uint8_t version : versions)
    {
        payload[0U] = version;
        const message_view raw = view_of(payload, sizeof(payload));
        EXPECT_TRUE(Message::matches(raw)); // matches() checks length and type only
        Message decoded = distinctive<Message>();
        EXPECT_FALSE(Message::decode(raw, decoded)) << "version " << static_cast<int>(version);
        expect_default(decoded);
    }
}

template <typename Message>
void expect_rejects_wrong_type()
{
    uint8_t payload[Message::payload_length] = {};
    fill_payload<Message>(payload);

    const uint8_t types[] = {0x00U, static_cast<uint8_t>(Message::message_type ^ 0x03U), 0xFFU};
    for (const uint8_t type : types)
    {
        payload[1U] = type;
        const message_view raw = view_of(payload, sizeof(payload));
        EXPECT_FALSE(Message::matches(raw)) << "type " << static_cast<int>(type);
        Message decoded = distinctive<Message>();
        EXPECT_FALSE(Message::decode(raw, decoded)) << "type " << static_cast<int>(type);
        expect_default(decoded);
    }
}

template <typename Message>
void expect_too_small_buffer_full()
{
    const Message value = distinctive<Message>();
    uint8_t frame[UBX_FRAME_OVERHEAD + Message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

template <typename Message>
void expect_beacon_id()
{
    Message value{};
    EXPECT_EQ(value.beacon_id(), 0U);

    // Big-endian: beacon[0] is the most significant byte.
    const uint8_t ordered[8U] = {0x0AU, 0x1BU, 0x2CU, 0x3DU, 0x4EU, 0x5FU, 0x60U, 0x71U};
    for (castle::size_type index = 0U; index < 8U; ++index)
    {
        value.beacon[index] = ordered[index];
    }
    EXPECT_EQ(value.beacon_id(), 0x0A1B2C3D4E5F6071ULL);

    // Only the least significant byte set.
    value = Message{};
    value.beacon[7U] = 0x01U;
    EXPECT_EQ(value.beacon_id(), 0x0000000000000001ULL);

    // Only beacon[1] set: lands in bits 55..48.
    value = Message{};
    value.beacon[1U] = 0x80U;
    EXPECT_EQ(value.beacon_id(), 0x0080000000000000ULL);

    // All ones: the top nibble of beacon[0] is masked off (60-bit identifier).
    for (uint8_t& byte : value.beacon)
    {
        byte = 0xFFU;
    }
    EXPECT_EQ(value.beacon_id(), 0x0FFFFFFFFFFFFFFFULL);

    // Only the top nibble set: nothing survives the mask.
    value = Message{};
    value.beacon[0U] = 0xF0U;
    EXPECT_EQ(value.beacon_id(), 0U);

    // Low nibble of beacon[0] is bits 59..56.
    value.beacon[0U] = 0xF5U;
    EXPECT_EQ(value.beacon_id(), 0x0500000000000000ULL);
}

// ---------------------------------------------------------------------------
// Shared constants
// ---------------------------------------------------------------------------

TEST(UbxMessageRxmRlm, SharedConstantsMatchSpec)
{
    EXPECT_EQ(RXM_RLM_TYPE_SHORT, 0x01U);
    EXPECT_EQ(RXM_RLM_TYPE_LONG, 0x02U);
    EXPECT_EQ(RXM_RLM_VERSION, 0x00U);
}

// ---------------------------------------------------------------------------
// rxm_rlm_short (16 bytes)
// ---------------------------------------------------------------------------

TEST(UbxMessageRxmRlm, ShortConstantsMatchSpec)
{
    EXPECT_EQ(rxm_rlm_short::msg_class, UBX_CLASS_RXM);
    EXPECT_EQ(rxm_rlm_short::msg_class, 0x02U);
    EXPECT_EQ(rxm_rlm_short::msg_id, UBX_ID_RXM_RLM);
    EXPECT_EQ(rxm_rlm_short::msg_id, 0x59U);
    EXPECT_EQ(rxm_rlm_short::payload_length, 16U);
    EXPECT_EQ(rxm_rlm_short::message_type, RXM_RLM_TYPE_SHORT);
    EXPECT_EQ(rxm_rlm_short::type_offset, 1U);
    EXPECT_EQ(rxm_rlm_short::BEACON_ID_MASK, 0x0FFFFFFFFFFFFFFFULL);
    const rxm_rlm_short value{};
    EXPECT_EQ(value.beacon.size(), 8U);
    EXPECT_EQ(value.params.size(), 2U);
}

TEST(UbxMessageRxmRlm, ShortRoundTripPreservesEveryField)
{
    expect_round_trip<rxm_rlm_short>();
}

TEST(UbxMessageRxmRlm, ShortDecodesFieldsAtSpecOffsets)
{
    uint8_t payload[rxm_rlm_short::payload_length] = {};
    fill_payload<rxm_rlm_short>(payload);
    EXPECT_EQ(payload[13U], 0xA0U);
    EXPECT_EQ(payload[14U], 0xA1U);
    EXPECT_EQ(payload[15U], 0xDDU);

    rxm_rlm_short decoded{};
    ASSERT_TRUE(rxm_rlm_short::decode(view_of(payload, sizeof(payload)), decoded));
    expect_spec_payload(decoded);
    EXPECT_EQ(decoded.params[0U], 0xA0U);
    EXPECT_EQ(decoded.params[1U], 0xA1U);
}

TEST(UbxMessageRxmRlm, ShortEncodesFieldsAtSpecOffsets)
{
    expect_encoded_at_spec_offsets<rxm_rlm_short>();
}

TEST(UbxMessageRxmRlm, ShortRejectsWrongClassOrId)
{
    expect_rejects_wrong_class_or_id<rxm_rlm_short>();
}

TEST(UbxMessageRxmRlm, ShortRejectsShortAndLongPayloads)
{
    expect_rejects_short_and_long<rxm_rlm_short>();
}

TEST(UbxMessageRxmRlm, ShortRejectsWrongVersion)
{
    expect_rejects_wrong_version<rxm_rlm_short>();
}

TEST(UbxMessageRxmRlm, ShortRejectsWrongType)
{
    expect_rejects_wrong_type<rxm_rlm_short>();
}

TEST(UbxMessageRxmRlm, ShortBeaconId)
{
    expect_beacon_id<rxm_rlm_short>();
}

TEST(UbxMessageRxmRlm, ShortEncodeIntoTooSmallBufferReportsFull)
{
    expect_too_small_buffer_full<rxm_rlm_short>();
}

// ---------------------------------------------------------------------------
// rxm_rlm_long (28 bytes)
// ---------------------------------------------------------------------------

TEST(UbxMessageRxmRlm, LongConstantsMatchSpec)
{
    EXPECT_EQ(rxm_rlm_long::msg_class, UBX_CLASS_RXM);
    EXPECT_EQ(rxm_rlm_long::msg_id, UBX_ID_RXM_RLM);
    EXPECT_EQ(rxm_rlm_long::payload_length, 28U);
    EXPECT_EQ(rxm_rlm_long::message_type, RXM_RLM_TYPE_LONG);
    EXPECT_EQ(rxm_rlm_long::type_offset, 1U);
    EXPECT_EQ(rxm_rlm_long::BEACON_ID_MASK, 0x0FFFFFFFFFFFFFFFULL);
    const rxm_rlm_long value{};
    EXPECT_EQ(value.beacon.size(), 8U);
    EXPECT_EQ(value.params.size(), 12U);
}

TEST(UbxMessageRxmRlm, LongRoundTripPreservesEveryField)
{
    expect_round_trip<rxm_rlm_long>();
}

TEST(UbxMessageRxmRlm, LongDecodesFieldsAtSpecOffsets)
{
    uint8_t payload[rxm_rlm_long::payload_length] = {};
    fill_payload<rxm_rlm_long>(payload);
    EXPECT_EQ(payload[13U], 0xA0U);
    EXPECT_EQ(payload[24U], 0xABU);
    EXPECT_EQ(payload[25U], 0xDDU);
    EXPECT_EQ(payload[27U], 0xDDU);

    rxm_rlm_long decoded{};
    ASSERT_TRUE(rxm_rlm_long::decode(view_of(payload, sizeof(payload)), decoded));
    expect_spec_payload(decoded);
    EXPECT_EQ(decoded.params[0U], 0xA0U);
    EXPECT_EQ(decoded.params[11U], 0xABU);
}

TEST(UbxMessageRxmRlm, LongEncodesFieldsAtSpecOffsets)
{
    expect_encoded_at_spec_offsets<rxm_rlm_long>();
}

TEST(UbxMessageRxmRlm, LongRejectsWrongClassOrId)
{
    expect_rejects_wrong_class_or_id<rxm_rlm_long>();
}

TEST(UbxMessageRxmRlm, LongRejectsShortAndLongPayloads)
{
    expect_rejects_short_and_long<rxm_rlm_long>();
}

TEST(UbxMessageRxmRlm, LongRejectsWrongVersion)
{
    expect_rejects_wrong_version<rxm_rlm_long>();
}

TEST(UbxMessageRxmRlm, LongRejectsWrongType)
{
    expect_rejects_wrong_type<rxm_rlm_long>();
}

TEST(UbxMessageRxmRlm, LongBeaconId)
{
    expect_beacon_id<rxm_rlm_long>();
}

TEST(UbxMessageRxmRlm, LongEncodeIntoTooSmallBufferReportsFull)
{
    expect_too_small_buffer_full<rxm_rlm_long>();
}

// ---------------------------------------------------------------------------
// Cross-layout discrimination
// ---------------------------------------------------------------------------

TEST(UbxMessageRxmRlm, MatchesDiscriminatesByLengthAndType)
{
    uint8_t payload[rxm_rlm_long::payload_length] = {};

    // Correct short frame: short only.
    fill_payload<rxm_rlm_short>(payload);
    EXPECT_TRUE(rxm_rlm_short::matches(view_of(payload, 16U)));
    EXPECT_FALSE(rxm_rlm_long::matches(view_of(payload, 16U)));

    // Short length carrying the long type: neither.
    payload[1U] = RXM_RLM_TYPE_LONG;
    EXPECT_FALSE(rxm_rlm_short::matches(view_of(payload, 16U)));
    EXPECT_FALSE(rxm_rlm_long::matches(view_of(payload, 16U)));

    // Correct long frame: long only.
    fill_payload<rxm_rlm_long>(payload);
    EXPECT_TRUE(rxm_rlm_long::matches(view_of(payload, 28U)));
    EXPECT_FALSE(rxm_rlm_short::matches(view_of(payload, 28U)));

    // Long length carrying the short type: neither.
    payload[1U] = RXM_RLM_TYPE_SHORT;
    EXPECT_FALSE(rxm_rlm_short::matches(view_of(payload, 28U)));
    EXPECT_FALSE(rxm_rlm_long::matches(view_of(payload, 28U)));
}

TEST(UbxMessageRxmRlm, LayoutsRejectEachOthersFrames)
{
    const rxm_rlm_short short_value = distinctive<rxm_rlm_short>();
    uint8_t short_frame[UBX_FRAME_OVERHEAD + rxm_rlm_short::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(short_value, short_frame, sizeof(short_frame), written), castle::status::ok);
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(short_frame, written), raw));
    rxm_rlm_long long_decoded = distinctive<rxm_rlm_long>();
    EXPECT_FALSE(rxm_rlm_long::matches(raw));
    EXPECT_FALSE(rxm_rlm_long::decode(raw, long_decoded));
    expect_default(long_decoded);

    const rxm_rlm_long long_value = distinctive<rxm_rlm_long>();
    uint8_t long_frame[UBX_FRAME_OVERHEAD + rxm_rlm_long::payload_length];
    ASSERT_EQ(encode_message(long_value, long_frame, sizeof(long_frame), written), castle::status::ok);
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(long_frame, written), raw));
    rxm_rlm_short short_decoded = distinctive<rxm_rlm_short>();
    EXPECT_FALSE(rxm_rlm_short::matches(raw));
    EXPECT_FALSE(rxm_rlm_short::decode(raw, short_decoded));
    expect_default(short_decoded);
}

} // namespace
