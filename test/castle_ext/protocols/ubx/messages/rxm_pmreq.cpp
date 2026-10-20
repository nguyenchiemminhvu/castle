#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/rxm_pmreq.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(const uint8_t* payload,
                     castle::size_type length,
                     uint8_t msg_class = UBX_CLASS_RXM,
                     uint8_t msg_id = UBX_ID_RXM_PMREQ)
{
    return message_view{{msg_class, msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

rxm_pmreq_v0 distinctive_v0()
{
    rxm_pmreq_v0 value{};
    value.duration = 0x11223344U;
    value.flags = 0xA5B6C7D8U;
    return value;
}

rxm_pmreq_v1 distinctive_v1()
{
    rxm_pmreq_v1 value{};
    value.duration = 0x0A1B2C3DU;
    value.flags = 0x5E6F7081U;
    value.wakeup_sources = 0x92A3B4C5U;
    return value;
}

void expect_default(const rxm_pmreq_v0& value)
{
    EXPECT_EQ(value.duration, RXM_PMREQ_DURATION_INFINITE);
    EXPECT_EQ(value.flags, 0U);
}

void expect_default(const rxm_pmreq_v1& value)
{
    EXPECT_EQ(value.duration, RXM_PMREQ_DURATION_INFINITE);
    EXPECT_EQ(value.flags, 0U);
    EXPECT_EQ(value.wakeup_sources, 0U);
}

// ---------------------------------------------------------------------------
// Shared constants
// ---------------------------------------------------------------------------

TEST(UbxMessageRxmPmreq, SharedConstantsMatchSpec)
{
    EXPECT_EQ(RXM_PMREQ_DURATION_INFINITE, 0U);
    EXPECT_EQ(RXM_PMREQ_VERSION, 0x00U);

    const rxm_pmreq_v0 v0{};
    EXPECT_EQ(v0.duration, RXM_PMREQ_DURATION_INFINITE);
    const rxm_pmreq_v1 v1{};
    EXPECT_EQ(v1.duration, RXM_PMREQ_DURATION_INFINITE);
}

// ---------------------------------------------------------------------------
// rxm_pmreq_v0 (8 bytes)
// ---------------------------------------------------------------------------

TEST(UbxMessageRxmPmreq, V0ConstantsMatchSpec)
{
    EXPECT_EQ(rxm_pmreq_v0::msg_class, UBX_CLASS_RXM);
    EXPECT_EQ(rxm_pmreq_v0::msg_class, 0x02U);
    EXPECT_EQ(rxm_pmreq_v0::msg_id, UBX_ID_RXM_PMREQ);
    EXPECT_EQ(rxm_pmreq_v0::msg_id, 0x41U);
    EXPECT_EQ(rxm_pmreq_v0::payload_length, 8U);
    EXPECT_EQ(rxm_pmreq_v0::FLAGS_BACKUP, 0x00000002U);
}

TEST(UbxMessageRxmPmreq, V0RoundTripPreservesEveryField)
{
    const rxm_pmreq_v0 original = distinctive_v0();
    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v0::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(rxm_pmreq_v0::matches(raw));
    EXPECT_FALSE(rxm_pmreq_v1::matches(raw));
    EXPECT_EQ(raw.payload_length(), rxm_pmreq_v0::payload_length);

    rxm_pmreq_v0 decoded{};
    ASSERT_TRUE(rxm_pmreq_v0::decode(raw, decoded));
    EXPECT_EQ(decoded.duration, original.duration);
    EXPECT_EQ(decoded.flags, original.flags);
}

TEST(UbxMessageRxmPmreq, V0DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[rxm_pmreq_v0::payload_length] = {};
    castle::write_le32(&payload[0U], 60000U);
    castle::write_le32(&payload[4U], 0x80000002U);

    rxm_pmreq_v0 decoded{};
    ASSERT_TRUE(rxm_pmreq_v0::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.duration, 60000U);
    EXPECT_EQ(decoded.flags, 0x80000002U);
    EXPECT_TRUE(decoded.backup());
}

TEST(UbxMessageRxmPmreq, V0EncodesFieldsAtSpecOffsets)
{
    const rxm_pmreq_v0 original = distinctive_v0();
    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v0::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_SYNC_1], UBX_SYNC_CHAR_1);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_SYNC_2], UBX_SYNC_CHAR_2);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_RXM);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_RXM_PMREQ);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 8U);
    EXPECT_EQ(castle::read_le32(&payload[0U]), 0x11223344U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), 0xA5B6C7D8U);
    EXPECT_EQ(payload[0U], 0x44U);
    EXPECT_EQ(payload[3U], 0x11U);
    EXPECT_EQ(payload[4U], 0xD8U);
    EXPECT_EQ(payload[7U], 0xA5U);
}

TEST(UbxMessageRxmPmreq, V0RejectsWrongClassOrId)
{
    uint8_t payload[rxm_pmreq_v0::payload_length] = {};
    castle::write_le32(&payload[0U], 7U);

    rxm_pmreq_v0 decoded = distinctive_v0();
    message_view raw = view_of(payload, sizeof(payload), UBX_CLASS_NAV, UBX_ID_RXM_PMREQ);
    EXPECT_FALSE(rxm_pmreq_v0::matches(raw));
    EXPECT_FALSE(rxm_pmreq_v0::decode(raw, decoded));
    expect_default(decoded);

    raw = view_of(payload, sizeof(payload), UBX_CLASS_RXM, UBX_ID_RXM_RTCM);
    EXPECT_FALSE(rxm_pmreq_v0::matches(raw));
    decoded = distinctive_v0();
    EXPECT_FALSE(rxm_pmreq_v0::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageRxmPmreq, V0MatchesOnlyItsExactLength)
{
    uint8_t payload[rxm_pmreq_v1::payload_length + 1U] = {};
    castle::write_le32(&payload[0U], 7U);

    EXPECT_TRUE(rxm_pmreq_v0::matches(view_of(payload, 8U)));
    EXPECT_FALSE(rxm_pmreq_v0::matches(view_of(payload, 7U)));
    EXPECT_FALSE(rxm_pmreq_v0::matches(view_of(payload, 9U)));
    EXPECT_FALSE(rxm_pmreq_v0::matches(view_of(payload, 16U)));
    EXPECT_FALSE(rxm_pmreq_v0::matches(view_of(payload, 0U)));
}

TEST(UbxMessageRxmPmreq, V0RejectsShortAndLongPayloads)
{
    uint8_t payload[rxm_pmreq_v1::payload_length + 1U] = {};
    castle::write_le32(&payload[0U], 7U);

    rxm_pmreq_v0 decoded = distinctive_v0();
    EXPECT_FALSE(rxm_pmreq_v0::decode(view_of(payload, rxm_pmreq_v0::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive_v0();
    EXPECT_FALSE(rxm_pmreq_v0::decode(view_of(payload, rxm_pmreq_v0::payload_length + 1U), decoded));
    expect_default(decoded);

    decoded = distinctive_v0();
    EXPECT_FALSE(rxm_pmreq_v0::decode(view_of(payload, 0U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageRxmPmreq, V0RejectsV1Frame)
{
    const rxm_pmreq_v1 request = distinctive_v1();
    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v1::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(request, frame, sizeof(frame), written), castle::status::ok);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_FALSE(rxm_pmreq_v0::matches(raw));

    rxm_pmreq_v0 decoded = distinctive_v0();
    EXPECT_FALSE(rxm_pmreq_v0::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageRxmPmreq, V0BackupAccessor)
{
    rxm_pmreq_v0 value{};
    EXPECT_FALSE(value.backup());

    value.flags = rxm_pmreq_v0::FLAGS_BACKUP;
    EXPECT_TRUE(value.backup());

    value.flags = ~rxm_pmreq_v0::FLAGS_BACKUP;
    EXPECT_FALSE(value.backup());

    value.flags = 0xFFFFFFFFU;
    EXPECT_TRUE(value.backup());
}

TEST(UbxMessageRxmPmreq, V0EncodeIntoTooSmallBufferReportsFull)
{
    const rxm_pmreq_v0 value = distinctive_v0();
    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v0::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

// ---------------------------------------------------------------------------
// rxm_pmreq_v1 (16 bytes)
// ---------------------------------------------------------------------------

TEST(UbxMessageRxmPmreq, V1ConstantsMatchSpec)
{
    EXPECT_EQ(rxm_pmreq_v1::msg_class, UBX_CLASS_RXM);
    EXPECT_EQ(rxm_pmreq_v1::msg_id, UBX_ID_RXM_PMREQ);
    EXPECT_EQ(rxm_pmreq_v1::payload_length, 16U);
    EXPECT_EQ(rxm_pmreq_v1::FLAGS_BACKUP, 0x00000002U);
    EXPECT_EQ(rxm_pmreq_v1::FLAGS_FORCE, 0x00000004U);
    EXPECT_EQ(rxm_pmreq_v1::WAKEUP_UARTRX, 0x00000008U);
    EXPECT_EQ(rxm_pmreq_v1::WAKEUP_EXTINT0, 0x00000020U);
    EXPECT_EQ(rxm_pmreq_v1::WAKEUP_EXTINT1, 0x00000040U);
    EXPECT_EQ(rxm_pmreq_v1::WAKEUP_SPICS, 0x00000080U);
}

TEST(UbxMessageRxmPmreq, V1RoundTripPreservesEveryField)
{
    const rxm_pmreq_v1 original = distinctive_v1();
    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v1::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(rxm_pmreq_v1::matches(raw));
    EXPECT_FALSE(rxm_pmreq_v0::matches(raw));
    EXPECT_EQ(raw.payload_length(), rxm_pmreq_v1::payload_length);

    rxm_pmreq_v1 decoded{};
    ASSERT_TRUE(rxm_pmreq_v1::decode(raw, decoded));
    EXPECT_EQ(decoded.duration, original.duration);
    EXPECT_EQ(decoded.flags, original.flags);
    EXPECT_EQ(decoded.wakeup_sources, original.wakeup_sources);
}

TEST(UbxMessageRxmPmreq, V1DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[rxm_pmreq_v1::payload_length] = {};
    payload[0U] = RXM_PMREQ_VERSION;
    payload[1U] = 0xAAU; // reserved0 is ignored on decode
    payload[2U] = 0xBBU;
    payload[3U] = 0xCCU;
    castle::write_le32(&payload[4U], 1036800000U);
    castle::write_le32(&payload[8U], 0x00000006U);
    castle::write_le32(&payload[12U], 0x000000E8U);

    rxm_pmreq_v1 decoded{};
    ASSERT_TRUE(rxm_pmreq_v1::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.duration, 1036800000U);
    EXPECT_EQ(decoded.flags, 0x00000006U);
    EXPECT_EQ(decoded.wakeup_sources, 0x000000E8U);
    EXPECT_TRUE(decoded.backup());
    EXPECT_TRUE(decoded.force());
    EXPECT_TRUE(decoded.wakeup_uartrx());
    EXPECT_TRUE(decoded.wakeup_extint0());
    EXPECT_TRUE(decoded.wakeup_extint1());
    EXPECT_TRUE(decoded.wakeup_spics());
}

TEST(UbxMessageRxmPmreq, V1EncodesFieldsAtSpecOffsets)
{
    const rxm_pmreq_v1 original = distinctive_v1();
    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v1::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_RXM);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_RXM_PMREQ);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 16U);
    EXPECT_EQ(payload[0U], 0x00U);
    EXPECT_EQ(payload[1U], 0x00U);
    EXPECT_EQ(payload[2U], 0x00U);
    EXPECT_EQ(payload[3U], 0x00U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), 0x0A1B2C3DU);
    EXPECT_EQ(castle::read_le32(&payload[8U]), 0x5E6F7081U);
    EXPECT_EQ(castle::read_le32(&payload[12U]), 0x92A3B4C5U);
    EXPECT_EQ(payload[4U], 0x3DU);
    EXPECT_EQ(payload[7U], 0x0AU);
    EXPECT_EQ(payload[8U], 0x81U);
    EXPECT_EQ(payload[11U], 0x5EU);
    EXPECT_EQ(payload[12U], 0xC5U);
    EXPECT_EQ(payload[15U], 0x92U);
}

TEST(UbxMessageRxmPmreq, V1RejectsWrongClassOrId)
{
    uint8_t payload[rxm_pmreq_v1::payload_length] = {};
    castle::write_le32(&payload[4U], 7U);

    rxm_pmreq_v1 decoded = distinctive_v1();
    message_view raw = view_of(payload, sizeof(payload), UBX_CLASS_NAV, UBX_ID_RXM_PMREQ);
    EXPECT_FALSE(rxm_pmreq_v1::matches(raw));
    EXPECT_FALSE(rxm_pmreq_v1::decode(raw, decoded));
    expect_default(decoded);

    raw = view_of(payload, sizeof(payload), UBX_CLASS_RXM, UBX_ID_RXM_RLM);
    EXPECT_FALSE(rxm_pmreq_v1::matches(raw));
    decoded = distinctive_v1();
    EXPECT_FALSE(rxm_pmreq_v1::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageRxmPmreq, V1MatchesOnlyItsExactLength)
{
    uint8_t payload[rxm_pmreq_v1::payload_length + 1U] = {};

    EXPECT_TRUE(rxm_pmreq_v1::matches(view_of(payload, 16U)));
    EXPECT_FALSE(rxm_pmreq_v1::matches(view_of(payload, 15U)));
    EXPECT_FALSE(rxm_pmreq_v1::matches(view_of(payload, 17U)));
    EXPECT_FALSE(rxm_pmreq_v1::matches(view_of(payload, 8U)));
    EXPECT_FALSE(rxm_pmreq_v1::matches(view_of(payload, 0U)));
}

TEST(UbxMessageRxmPmreq, V1RejectsShortAndLongPayloads)
{
    uint8_t payload[rxm_pmreq_v1::payload_length + 1U] = {};
    castle::write_le32(&payload[4U], 7U);

    rxm_pmreq_v1 decoded = distinctive_v1();
    EXPECT_FALSE(rxm_pmreq_v1::decode(view_of(payload, rxm_pmreq_v1::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive_v1();
    EXPECT_FALSE(rxm_pmreq_v1::decode(view_of(payload, rxm_pmreq_v1::payload_length + 1U), decoded));
    expect_default(decoded);

    decoded = distinctive_v1();
    EXPECT_FALSE(rxm_pmreq_v1::decode(view_of(payload, 0U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageRxmPmreq, V1RejectsV0Frame)
{
    const rxm_pmreq_v0 request = distinctive_v0();
    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v0::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(request, frame, sizeof(frame), written), castle::status::ok);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_FALSE(rxm_pmreq_v1::matches(raw));

    rxm_pmreq_v1 decoded = distinctive_v1();
    EXPECT_FALSE(rxm_pmreq_v1::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageRxmPmreq, V1RejectsNonZeroVersion)
{
    uint8_t payload[rxm_pmreq_v1::payload_length] = {};
    castle::write_le32(&payload[4U], 1000U);
    castle::write_le32(&payload[8U], rxm_pmreq_v1::FLAGS_BACKUP);
    castle::write_le32(&payload[12U], rxm_pmreq_v1::WAKEUP_UARTRX);

    rxm_pmreq_v1 decoded{};
    ASSERT_TRUE(rxm_pmreq_v1::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.duration, 1000U);

    const uint8_t versions[] = {0x01U, 0x02U, 0x80U, 0xFFU};
    for (const uint8_t version : versions)
    {
        payload[0U] = version;
        const message_view raw = view_of(payload, sizeof(payload));
        EXPECT_TRUE(rxm_pmreq_v1::matches(raw)); // matches() checks length only
        decoded = distinctive_v1();
        EXPECT_FALSE(rxm_pmreq_v1::decode(raw, decoded)) << "version " << static_cast<int>(version);
        expect_default(decoded);
    }
}

TEST(UbxMessageRxmPmreq, V1FlagAccessors)
{
    rxm_pmreq_v1 value{};
    EXPECT_FALSE(value.backup());
    EXPECT_FALSE(value.force());

    value.flags = rxm_pmreq_v1::FLAGS_BACKUP;
    EXPECT_TRUE(value.backup());
    EXPECT_FALSE(value.force());

    value.flags = rxm_pmreq_v1::FLAGS_FORCE;
    EXPECT_FALSE(value.backup());
    EXPECT_TRUE(value.force());

    value.flags = rxm_pmreq_v1::FLAGS_BACKUP | rxm_pmreq_v1::FLAGS_FORCE;
    EXPECT_TRUE(value.backup());
    EXPECT_TRUE(value.force());

    value.flags = ~(rxm_pmreq_v1::FLAGS_BACKUP | rxm_pmreq_v1::FLAGS_FORCE);
    EXPECT_FALSE(value.backup());
    EXPECT_FALSE(value.force());

    // Flags never leak into wake-up accessors.
    value.flags = 0xFFFFFFFFU;
    EXPECT_FALSE(value.wakeup_uartrx());
    EXPECT_FALSE(value.wakeup_extint0());
    EXPECT_FALSE(value.wakeup_extint1());
    EXPECT_FALSE(value.wakeup_spics());
}

TEST(UbxMessageRxmPmreq, V1WakeupAccessors)
{
    rxm_pmreq_v1 value{};
    EXPECT_FALSE(value.wakeup_uartrx());
    EXPECT_FALSE(value.wakeup_extint0());
    EXPECT_FALSE(value.wakeup_extint1());
    EXPECT_FALSE(value.wakeup_spics());

    value.wakeup_sources = rxm_pmreq_v1::WAKEUP_UARTRX;
    EXPECT_TRUE(value.wakeup_uartrx());
    EXPECT_FALSE(value.wakeup_extint0());
    EXPECT_FALSE(value.wakeup_extint1());
    EXPECT_FALSE(value.wakeup_spics());

    value.wakeup_sources = rxm_pmreq_v1::WAKEUP_EXTINT0;
    EXPECT_FALSE(value.wakeup_uartrx());
    EXPECT_TRUE(value.wakeup_extint0());
    EXPECT_FALSE(value.wakeup_extint1());
    EXPECT_FALSE(value.wakeup_spics());

    value.wakeup_sources = rxm_pmreq_v1::WAKEUP_EXTINT1;
    EXPECT_FALSE(value.wakeup_uartrx());
    EXPECT_FALSE(value.wakeup_extint0());
    EXPECT_TRUE(value.wakeup_extint1());
    EXPECT_FALSE(value.wakeup_spics());

    value.wakeup_sources = rxm_pmreq_v1::WAKEUP_SPICS;
    EXPECT_FALSE(value.wakeup_uartrx());
    EXPECT_FALSE(value.wakeup_extint0());
    EXPECT_FALSE(value.wakeup_extint1());
    EXPECT_TRUE(value.wakeup_spics());

    value.wakeup_sources = ~(rxm_pmreq_v1::WAKEUP_UARTRX | rxm_pmreq_v1::WAKEUP_EXTINT0
                             | rxm_pmreq_v1::WAKEUP_EXTINT1 | rxm_pmreq_v1::WAKEUP_SPICS);
    EXPECT_FALSE(value.wakeup_uartrx());
    EXPECT_FALSE(value.wakeup_extint0());
    EXPECT_FALSE(value.wakeup_extint1());
    EXPECT_FALSE(value.wakeup_spics());

    // Wake-up sources never leak into flag accessors.
    value.wakeup_sources = 0xFFFFFFFFU;
    EXPECT_TRUE(value.wakeup_uartrx());
    EXPECT_TRUE(value.wakeup_extint0());
    EXPECT_TRUE(value.wakeup_extint1());
    EXPECT_TRUE(value.wakeup_spics());
    EXPECT_FALSE(value.backup());
    EXPECT_FALSE(value.force());
}

TEST(UbxMessageRxmPmreq, V1InfiniteDurationEncodesAsZero)
{
    rxm_pmreq_v1 request = distinctive_v1();
    request.duration = RXM_PMREQ_DURATION_INFINITE;
    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v1::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(request, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(castle::read_le32(&frame[UBX_FRAME_INDEX_PAYLOAD + 4U]), 0U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    rxm_pmreq_v1 decoded = distinctive_v1();
    ASSERT_TRUE(rxm_pmreq_v1::decode(raw, decoded));
    EXPECT_EQ(decoded.duration, RXM_PMREQ_DURATION_INFINITE);
}

TEST(UbxMessageRxmPmreq, V1EncodeIntoTooSmallBufferReportsFull)
{
    const rxm_pmreq_v1 value = distinctive_v1();
    uint8_t frame[UBX_FRAME_OVERHEAD + rxm_pmreq_v1::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
