#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/upd_sos.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{UBX_CLASS_UPD, UBX_ID_UPD_SOS, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

struct match_row
{
    bool save;
    bool clear;
    bool ack;
    bool restored;
};

match_row matches_of(const message_view& raw)
{
    return match_row{upd_sos_save::matches(raw),
                     upd_sos_clear::matches(raw),
                     upd_sos_ack::matches(raw),
                     upd_sos_restored::matches(raw)};
}

/** Encodes `value`, re-parses the frame and returns the matches() row. */
template <typename Message>
match_row encoded_matches(const Message& value)
{
    uint8_t frame[UBX_FRAME_OVERHEAD + 8U];
    castle::size_type written = 0U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD + Message::payload_length);
    message_view raw{};
    EXPECT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    return matches_of(raw);
}

/** Both 8-byte outputs reject the view and reset `out` to default. */
void expect_ack_and_restored_reject(const message_view& raw)
{
    upd_sos_ack ack{};
    ack.response = upd_sos_ack_response::acknowledged;
    EXPECT_FALSE(upd_sos_ack::decode(raw, ack));
    EXPECT_EQ(ack.response, upd_sos_ack_response::not_acknowledged);

    upd_sos_restored restored{};
    restored.response = upd_sos_restore_response::not_restored;
    EXPECT_FALSE(upd_sos_restored::decode(raw, restored));
    EXPECT_EQ(restored.response, upd_sos_restore_response::unknown);
}

template <typename Message>
void expect_full_on_small_buffer(const Message& value)
{
    uint8_t frame[UBX_FRAME_OVERHEAD + 8U];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD + Message::payload_length - 1U, written),
              castle::status::full);
    EXPECT_EQ(written, 0U);

    written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageUpdSos, ConstantsMatchSpec)
{
    EXPECT_EQ(upd_sos_save::msg_class, 0x09U);
    EXPECT_EQ(upd_sos_save::msg_id, 0x14U);
    EXPECT_EQ(upd_sos_save::msg_class, UBX_CLASS_UPD);
    EXPECT_EQ(upd_sos_save::msg_id, UBX_ID_UPD_SOS);
    EXPECT_EQ(upd_sos_clear::msg_class, UBX_CLASS_UPD);
    EXPECT_EQ(upd_sos_clear::msg_id, UBX_ID_UPD_SOS);
    EXPECT_EQ(upd_sos_ack::msg_class, UBX_CLASS_UPD);
    EXPECT_EQ(upd_sos_ack::msg_id, UBX_ID_UPD_SOS);
    EXPECT_EQ(upd_sos_restored::msg_class, UBX_CLASS_UPD);
    EXPECT_EQ(upd_sos_restored::msg_id, UBX_ID_UPD_SOS);

    EXPECT_EQ(upd_sos_save::payload_length, 4U);
    EXPECT_EQ(upd_sos_clear::payload_length, 4U);
    EXPECT_EQ(upd_sos_ack::payload_length, 8U);
    EXPECT_EQ(upd_sos_restored::payload_length, 8U);

    EXPECT_EQ(upd_sos_save::message_type, 0U);
    EXPECT_EQ(upd_sos_clear::message_type, 1U);
    EXPECT_EQ(upd_sos_ack::message_type, 2U);
    EXPECT_EQ(upd_sos_restored::message_type, 3U);
}

TEST(UbxMessageUpdSos, EnumValuesMatchSpec)
{
    EXPECT_EQ(static_cast<uint8_t>(upd_sos_cmd::save), 0U);
    EXPECT_EQ(static_cast<uint8_t>(upd_sos_cmd::clear), 1U);
    EXPECT_EQ(static_cast<uint8_t>(upd_sos_cmd::ack), 2U);
    EXPECT_EQ(static_cast<uint8_t>(upd_sos_cmd::restored), 3U);
    EXPECT_EQ(static_cast<uint8_t>(upd_sos_ack_response::not_acknowledged), 0U);
    EXPECT_EQ(static_cast<uint8_t>(upd_sos_ack_response::acknowledged), 1U);
    EXPECT_EQ(static_cast<uint8_t>(upd_sos_restore_response::unknown), 0U);
    EXPECT_EQ(static_cast<uint8_t>(upd_sos_restore_response::failed), 1U);
    EXPECT_EQ(static_cast<uint8_t>(upd_sos_restore_response::restored), 2U);
    EXPECT_EQ(static_cast<uint8_t>(upd_sos_restore_response::not_restored), 3U);
}

TEST(UbxMessageUpdSos, EachEncodedFrameMatchesExactlyOneLayout)
{
    upd_sos_ack ack{};
    ack.response = upd_sos_ack_response::acknowledged;
    upd_sos_restored restored{};
    restored.response = upd_sos_restore_response::restored;

    const match_row rows[4U] = {
        encoded_matches(upd_sos_save{}),
        encoded_matches(upd_sos_clear{}),
        encoded_matches(ack),
        encoded_matches(restored),
    };

    for (castle::size_type i = 0U; i < 4U; ++i)
    {
        const bool row[4U] = {rows[i].save, rows[i].clear, rows[i].ack, rows[i].restored};
        for (castle::size_type j = 0U; j < 4U; ++j)
        {
            EXPECT_EQ(row[j], i == j) << "frame " << i << " layout " << j;
        }
    }
}

TEST(UbxMessageUpdSos, SaveEncodesAndRoundTrips)
{
    uint8_t frame[UBX_FRAME_OVERHEAD + upd_sos_save::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(upd_sos_save{}, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_UPD);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_UPD_SOS);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 4U);
    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[0U], 0U);
    EXPECT_EQ(payload[1U], 0U);
    EXPECT_EQ(payload[2U], 0U);
    EXPECT_EQ(payload[3U], 0U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    upd_sos_save decoded{};
    EXPECT_TRUE(upd_sos_save::decode(raw, decoded));
}

TEST(UbxMessageUpdSos, ClearEncodesAndRoundTrips)
{
    uint8_t frame[UBX_FRAME_OVERHEAD + upd_sos_clear::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(upd_sos_clear{}, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 4U);
    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[0U], 1U);
    EXPECT_EQ(payload[1U], 0U);
    EXPECT_EQ(payload[2U], 0U);
    EXPECT_EQ(payload[3U], 0U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    upd_sos_clear decoded{};
    EXPECT_TRUE(upd_sos_clear::decode(raw, decoded));
}

TEST(UbxMessageUpdSos, AckEncodesAtSpecOffsetsAndRoundTripsAllResponses)
{
    const upd_sos_ack_response responses[] = {upd_sos_ack_response::not_acknowledged,
                                              upd_sos_ack_response::acknowledged};
    for (upd_sos_ack_response response : responses)
    {
        upd_sos_ack original{};
        original.response = response;
        uint8_t frame[UBX_FRAME_OVERHEAD + upd_sos_ack::payload_length];
        castle::size_type written = 0U;
        ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
        EXPECT_EQ(written, sizeof(frame));
        EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 8U);
        const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
        EXPECT_EQ(payload[0U], 2U);
        EXPECT_EQ(payload[1U], 0U);
        EXPECT_EQ(payload[2U], 0U);
        EXPECT_EQ(payload[3U], 0U);
        EXPECT_EQ(payload[4U], static_cast<uint8_t>(response));
        EXPECT_EQ(payload[5U], 0U);
        EXPECT_EQ(payload[6U], 0U);
        EXPECT_EQ(payload[7U], 0U);

        message_view raw{};
        ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
        upd_sos_ack decoded{};
        decoded.response = (response == upd_sos_ack_response::acknowledged)
                               ? upd_sos_ack_response::not_acknowledged
                               : upd_sos_ack_response::acknowledged;
        ASSERT_TRUE(upd_sos_ack::decode(raw, decoded));
        EXPECT_EQ(decoded.response, response);
    }
}

TEST(UbxMessageUpdSos, RestoredEncodesAtSpecOffsetsAndRoundTripsAllResponses)
{
    const upd_sos_restore_response responses[] = {upd_sos_restore_response::unknown,
                                                  upd_sos_restore_response::failed,
                                                  upd_sos_restore_response::restored,
                                                  upd_sos_restore_response::not_restored};
    for (upd_sos_restore_response response : responses)
    {
        upd_sos_restored original{};
        original.response = response;
        uint8_t frame[UBX_FRAME_OVERHEAD + upd_sos_restored::payload_length];
        castle::size_type written = 0U;
        ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
        EXPECT_EQ(written, sizeof(frame));
        EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 8U);
        const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
        EXPECT_EQ(payload[0U], 3U);
        EXPECT_EQ(payload[1U], 0U);
        EXPECT_EQ(payload[2U], 0U);
        EXPECT_EQ(payload[3U], 0U);
        EXPECT_EQ(payload[4U], static_cast<uint8_t>(response));
        EXPECT_EQ(payload[5U], 0U);
        EXPECT_EQ(payload[6U], 0U);
        EXPECT_EQ(payload[7U], 0U);

        message_view raw{};
        ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
        upd_sos_restored decoded{};
        decoded.response = (response == upd_sos_restore_response::failed) ? upd_sos_restore_response::restored
                                                                         : upd_sos_restore_response::failed;
        ASSERT_TRUE(upd_sos_restored::decode(raw, decoded));
        EXPECT_EQ(decoded.response, response);
    }
}

TEST(UbxMessageUpdSos, DecodesHandBuiltOutputsAtSpecOffsets)
{
    uint8_t payload[8U] = {0x02U, 0xAAU, 0xBBU, 0xCCU, 0x01U, 0xDDU, 0xEEU, 0xFFU};
    upd_sos_ack ack{};
    ASSERT_TRUE(upd_sos_ack::matches(view_of(payload, sizeof(payload))));
    ASSERT_TRUE(upd_sos_ack::decode(view_of(payload, sizeof(payload)), ack));
    EXPECT_EQ(ack.response, upd_sos_ack_response::acknowledged);

    payload[0U] = 0x03U;
    payload[4U] = 0x03U;
    upd_sos_restored restored{};
    ASSERT_TRUE(upd_sos_restored::matches(view_of(payload, sizeof(payload))));
    ASSERT_TRUE(upd_sos_restored::decode(view_of(payload, sizeof(payload)), restored));
    EXPECT_EQ(restored.response, upd_sos_restore_response::not_restored);

    // Raw response values without an enumerator pass through unchanged.
    payload[4U] = 0x7FU;
    ASSERT_TRUE(upd_sos_restored::decode(view_of(payload, sizeof(payload)), restored));
    EXPECT_EQ(static_cast<uint8_t>(restored.response), 0x7FU);

    // Reserved bytes are ignored on decode for the 4-byte commands as well.
    const uint8_t save_payload[4U] = {0x00U, 0x11U, 0x22U, 0x33U};
    upd_sos_save save{};
    EXPECT_TRUE(upd_sos_save::decode(view_of(save_payload, sizeof(save_payload)), save));
    const uint8_t clear_payload[4U] = {0x01U, 0x11U, 0x22U, 0x33U};
    upd_sos_clear clear{};
    EXPECT_TRUE(upd_sos_clear::decode(view_of(clear_payload, sizeof(clear_payload)), clear));
}

TEST(UbxMessageUpdSos, DecodeRejectsCorrectLengthWithWrongCmd)
{
    uint8_t long_payload[8U] = {0x03U, 0U, 0U, 0U, 0x01U, 0U, 0U, 0U};
    upd_sos_ack ack{};
    ack.response = upd_sos_ack_response::acknowledged;
    EXPECT_FALSE(upd_sos_ack::matches(view_of(long_payload, sizeof(long_payload))));
    EXPECT_FALSE(upd_sos_ack::decode(view_of(long_payload, sizeof(long_payload)), ack));
    EXPECT_EQ(ack.response, upd_sos_ack_response::not_acknowledged);

    long_payload[0U] = 0x02U;
    upd_sos_restored restored{};
    restored.response = upd_sos_restore_response::failed;
    EXPECT_FALSE(upd_sos_restored::matches(view_of(long_payload, sizeof(long_payload))));
    EXPECT_FALSE(upd_sos_restored::decode(view_of(long_payload, sizeof(long_payload)), restored));
    EXPECT_EQ(restored.response, upd_sos_restore_response::unknown);

    const uint8_t clear_payload[4U] = {0x01U, 0U, 0U, 0U};
    upd_sos_save save{};
    EXPECT_FALSE(upd_sos_save::matches(view_of(clear_payload, sizeof(clear_payload))));
    EXPECT_FALSE(upd_sos_save::decode(view_of(clear_payload, sizeof(clear_payload)), save));

    const uint8_t save_payload[4U] = {0x00U, 0U, 0U, 0U};
    upd_sos_clear clear{};
    EXPECT_FALSE(upd_sos_clear::matches(view_of(save_payload, sizeof(save_payload))));
    EXPECT_FALSE(upd_sos_clear::decode(view_of(save_payload, sizeof(save_payload)), clear));

    // A cmd byte outside the enumeration matches nothing.
    const uint8_t bogus4[4U] = {0x04U, 0U, 0U, 0U};
    const match_row row4 = matches_of(view_of(bogus4, sizeof(bogus4)));
    EXPECT_FALSE(row4.save || row4.clear || row4.ack || row4.restored);
    const uint8_t bogus8[8U] = {0xFFU, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    const match_row row8 = matches_of(view_of(bogus8, sizeof(bogus8)));
    EXPECT_FALSE(row8.save || row8.clear || row8.ack || row8.restored);
}

TEST(UbxMessageUpdSos, RejectsWrongClassOrId)
{
    const uint8_t payload[8U] = {0x02U, 0U, 0U, 0U, 0x01U, 0U, 0U, 0U};
    message_view raw = view_of(payload, sizeof(payload));
    raw.header.msg_class = UBX_CLASS_MON;
    EXPECT_FALSE(upd_sos_ack::matches(raw));
    expect_ack_and_restored_reject(raw);

    raw.header.msg_class = UBX_CLASS_UPD;
    raw.header.msg_id = 0x15U;
    EXPECT_FALSE(upd_sos_ack::matches(raw));
    expect_ack_and_restored_reject(raw);

    const uint8_t short_payload[4U] = {0x00U, 0U, 0U, 0U};
    raw = view_of(short_payload, sizeof(short_payload));
    raw.header.msg_class = UBX_CLASS_TIM;
    upd_sos_save save{};
    EXPECT_FALSE(upd_sos_save::matches(raw));
    EXPECT_FALSE(upd_sos_save::decode(raw, save));
    raw.header.msg_class = UBX_CLASS_UPD;
    raw.header.msg_id = 0x00U;
    upd_sos_clear clear{};
    EXPECT_FALSE(upd_sos_clear::matches(raw));
    EXPECT_FALSE(upd_sos_clear::decode(raw, clear));
}

TEST(UbxMessageUpdSos, RejectsShortLongAndZeroLengthPayloads)
{
    uint8_t ack_payload[9U] = {0x02U, 0U, 0U, 0U, 0x01U, 0U, 0U, 0U, 0U};
    uint8_t restored_payload[9U] = {0x03U, 0U, 0U, 0U, 0x02U, 0U, 0U, 0U, 0U};
    const castle::size_type bad_lengths[] = {7U, 9U, 0U, 4U};
    for (castle::size_type length : bad_lengths)
    {
        upd_sos_ack ack{};
        ack.response = upd_sos_ack_response::acknowledged;
        EXPECT_FALSE(upd_sos_ack::matches(view_of(ack_payload, length))) << "length " << length;
        EXPECT_FALSE(upd_sos_ack::decode(view_of(ack_payload, length), ack)) << "length " << length;
        EXPECT_EQ(ack.response, upd_sos_ack_response::not_acknowledged);

        upd_sos_restored restored{};
        restored.response = upd_sos_restore_response::failed;
        EXPECT_FALSE(upd_sos_restored::matches(view_of(restored_payload, length))) << "length " << length;
        EXPECT_FALSE(upd_sos_restored::decode(view_of(restored_payload, length), restored)) << "length " << length;
        EXPECT_EQ(restored.response, upd_sos_restore_response::unknown);
    }

    uint8_t save_payload[8U] = {0x00U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    uint8_t clear_payload[8U] = {0x01U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    const castle::size_type bad_command_lengths[] = {3U, 5U, 0U, 8U};
    for (castle::size_type length : bad_command_lengths)
    {
        upd_sos_save save{};
        EXPECT_FALSE(upd_sos_save::matches(view_of(save_payload, length))) << "length " << length;
        EXPECT_FALSE(upd_sos_save::decode(view_of(save_payload, length), save)) << "length " << length;
        upd_sos_clear clear{};
        EXPECT_FALSE(upd_sos_clear::matches(view_of(clear_payload, length))) << "length " << length;
        EXPECT_FALSE(upd_sos_clear::decode(view_of(clear_payload, length), clear)) << "length " << length;
    }
}

TEST(UbxMessageUpdSos, PollIsEmptyFrameMatchingNoLayout)
{
    uint8_t frame[UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_poll<upd_sos_restored>(frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_UPD);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_UPD_SOS);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 0U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_EQ(raw.payload_length(), 0U);
    const match_row row = matches_of(raw);
    EXPECT_FALSE(row.save);
    EXPECT_FALSE(row.clear);
    EXPECT_FALSE(row.ack);
    EXPECT_FALSE(row.restored);
    expect_ack_and_restored_reject(raw);

    written = 99U;
    EXPECT_EQ(encode_poll<upd_sos_restored>(frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageUpdSos, EncodeIntoTooSmallBufferReportsFull)
{
    expect_full_on_small_buffer(upd_sos_save{});
    expect_full_on_small_buffer(upd_sos_clear{});
    upd_sos_ack ack{};
    ack.response = upd_sos_ack_response::acknowledged;
    expect_full_on_small_buffer(ack);
    upd_sos_restored restored{};
    restored.response = upd_sos_restore_response::restored;
    expect_full_on_small_buffer(restored);
}

} // namespace
