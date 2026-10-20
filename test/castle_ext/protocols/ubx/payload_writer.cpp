#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"

namespace
{

using namespace castle::protocols::ubx;
using castle::container::array_view;

enum class wire_enum : uint16_t
{
    first = 0x0102U,
    second = 0xA0B0U
};

struct layout_message
{
    static CASTLE_CONSTEXPR uint8_t msg_class = 0x0AU;
    static CASTLE_CONSTEXPR uint8_t msg_id = 0x55U;

    wire_enum kind = wire_enum::first;
    double value = 0.0;
    castle::container::array<int8_t, 3U> trio{};
    castle::container::string<4U> label{};

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(0x7FU);
        stream.field(self.kind);
        stream.field(self.value);
        stream.field(self.trio);
        stream.reserved(2U);
        stream.text(self.label);
    }

    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        layout(*this, writer);
    }
};

/** @brief Encoder that always latches a non-capacity failure. */
struct failing_message
{
    static CASTLE_CONSTEXPR uint8_t msg_class = 0x06U;
    static CASTLE_CONSTEXPR uint8_t msg_id = 0x8AU;

    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.write(static_cast<uint8_t>(1U));
        writer.fail(castle::status::invalid_argument);
    }
};

/** @brief Encoder producing more than UBX_MAX_PAYLOAD_LEN bytes. */
struct oversized_message
{
    static CASTLE_CONSTEXPR uint8_t msg_class = 0x02U;
    static CASTLE_CONSTEXPR uint8_t msg_id = 0x15U;

    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.write_zeros(static_cast<castle::size_type>(UBX_MAX_PAYLOAD_LEN) + 1U);
    }
};

TEST(UbxPayloadWriter, WritesEveryScalarWidthLittleEndian)
{
    uint8_t buffer[44U] = {};
    payload_writer writer(array_view<uint8_t>(buffer, sizeof(buffer)));

    writer.write('Z');
    writer.write(static_cast<uint8_t>(0xF1U));
    writer.write(static_cast<int8_t>(-3));
    writer.write(static_cast<uint16_t>(0x1234U));
    writer.write(static_cast<int16_t>(-2));
    writer.write(static_cast<uint32_t>(0x89ABCDEFU));
    writer.write(static_cast<int32_t>(-5));
    writer.write(static_cast<uint64_t>(0x0123456789ABCDEFULL));
    writer.write(static_cast<int64_t>(-7));
    writer.write(-0.25F);
    writer.write(3.0);
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.size(), 43U);
    EXPECT_EQ(writer.remaining(), 1U);

    EXPECT_EQ(buffer[0U], static_cast<uint8_t>('Z'));
    EXPECT_EQ(buffer[1U], 0xF1U);
    EXPECT_EQ(buffer[2U], 0xFDU);
    EXPECT_EQ(castle::read_le16(&buffer[3U]), 0x1234U);
    EXPECT_EQ(castle::read_le16s(&buffer[5U]), -2);
    EXPECT_EQ(castle::read_le32(&buffer[7U]), 0x89ABCDEFU);
    EXPECT_EQ(castle::read_le32s(&buffer[11U]), -5);
    EXPECT_EQ(castle::read_le64(&buffer[15U]), 0x0123456789ABCDEFULL);
    EXPECT_EQ(castle::read_le64s(&buffer[23U]), -7);

    payload_reader reader(array_view<CASTLE_CONST uint8_t>(&buffer[31U], 12U));
    EXPECT_FLOAT_EQ(reader.read_r4(), -0.25F);
    EXPECT_DOUBLE_EQ(reader.read_r8(), 3.0);
    EXPECT_TRUE(reader.ok());
}

TEST(UbxPayloadWriter, RawHelpersWriteExactBytes)
{
    uint8_t buffer[9U];
    for (uint8_t& byte : buffer)
    {
        byte = 0xEEU;
    }
    payload_writer writer(array_view<uint8_t>(buffer, sizeof(buffer)));

    writer.write_zeros(2U);
    CASTLE_CONST uint8_t source[3U] = {0x10U, 0x20U, 0x30U};
    writer.write_bytes(source, 3U);
    writer.write_uint(0x00C0FFEEULL, 3U);
    writer.write_bytes(nullptr, 0U);
    writer.write_uint(0xFFU, 0U);

    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.size(), 8U);
    EXPECT_EQ(buffer[0U], 0U);
    EXPECT_EQ(buffer[1U], 0U);
    EXPECT_EQ(buffer[2U], 0x10U);
    EXPECT_EQ(buffer[4U], 0x30U);
    EXPECT_EQ(buffer[5U], 0xEEU);
    EXPECT_EQ(buffer[6U], 0xFFU);
    EXPECT_EQ(buffer[7U], 0xC0U);
    EXPECT_EQ(buffer[8U], 0xEEU); // untouched
}

TEST(UbxPayloadWriter, OverflowLatchesFullAndStopsWriting)
{
    uint8_t buffer[3U] = {0xAAU, 0xAAU, 0xAAU};
    payload_writer writer(array_view<uint8_t>(buffer, sizeof(buffer)));

    writer.write(static_cast<uint16_t>(0x0102U));
    writer.write(static_cast<uint16_t>(0x0304U)); // needs 2, only 1 left
    EXPECT_FALSE(writer.ok());
    EXPECT_EQ(writer.status(), castle::status::full);
    EXPECT_EQ(writer.size(), 2U);
    EXPECT_EQ(buffer[2U], 0xAAU);

    writer.write(static_cast<uint8_t>(0x55U)); // would fit, but the error is sticky
    writer.write_zeros(1U);
    writer.write_bytes(buffer, 1U);
    EXPECT_EQ(writer.size(), 2U);
    EXPECT_EQ(buffer[2U], 0xAAU);

    writer.fail(castle::status::invalid_argument);
    EXPECT_EQ(writer.status(), castle::status::full); // first failure wins
}

TEST(UbxPayloadWriter, FailKeepsFirstReason)
{
    uint8_t buffer[4U] = {};
    payload_writer writer(array_view<uint8_t>(buffer, sizeof(buffer)));
    EXPECT_EQ(writer.status(), castle::status::ok);

    writer.fail(castle::status::invalid_argument);
    writer.fail(castle::status::out_of_range);
    EXPECT_EQ(writer.status(), castle::status::invalid_argument);

    writer.write(static_cast<uint8_t>(1U));
    EXPECT_EQ(writer.size(), 0U);
}

TEST(UbxPayloadWriter, NullViewHasZeroCapacity)
{
    payload_writer empty{array_view<uint8_t>{}};
    EXPECT_TRUE(empty.ok());
    EXPECT_EQ(empty.remaining(), 0U);
    empty.write_zeros(0U);
    EXPECT_TRUE(empty.ok());
    empty.write(static_cast<uint8_t>(1U));
    EXPECT_EQ(empty.status(), castle::status::full);

    payload_writer dangling{array_view<uint8_t>(nullptr, 8U)};
    EXPECT_EQ(dangling.remaining(), 0U);
    dangling.write(static_cast<uint8_t>(1U));
    EXPECT_EQ(dangling.status(), castle::status::full);
}

TEST(UbxPayloadWriter, LayoutVerbsRoundTripThroughReader)
{
    layout_message in{};
    in.kind = wire_enum::second;
    in.value = -12.5;
    in.trio[0U] = -1;
    in.trio[1U] = 2;
    in.trio[2U] = -3;
    ASSERT_EQ(in.label.assign("AB"), castle::status::ok);

    uint8_t buffer[20U] = {};
    payload_writer writer(array_view<uint8_t>(buffer, sizeof(buffer)));
    in.encode(writer);
    ASSERT_TRUE(writer.ok());
    ASSERT_EQ(writer.size(), 20U);

    // Wire offsets: 0 constant, 1 enum U2, 3 R8, 11 I1[3], 14 reserved[2], 16 CH[4].
    EXPECT_EQ(buffer[0U], 0x7FU);
    EXPECT_EQ(castle::read_le16(&buffer[1U]), 0xA0B0U);
    EXPECT_EQ(static_cast<int8_t>(buffer[13U]), -3);
    EXPECT_EQ(buffer[14U], 0U);
    EXPECT_EQ(buffer[15U], 0U);
    EXPECT_EQ(buffer[16U], static_cast<uint8_t>('A'));
    EXPECT_EQ(buffer[17U], static_cast<uint8_t>('B'));
    EXPECT_EQ(buffer[18U], 0U);
    EXPECT_EQ(buffer[19U], 0U);

    layout_message out{};
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(buffer, sizeof(buffer)));
    layout_message::layout(out, reader);
    ASSERT_TRUE(reader.ok());
    EXPECT_EQ(out.kind, wire_enum::second);
    EXPECT_DOUBLE_EQ(out.value, -12.5);
    EXPECT_EQ(out.trio[0U], -1);
    EXPECT_EQ(out.trio[2U], -3);
    EXPECT_TRUE(out.label.view() == in.label.view());
    EXPECT_EQ(reader.remaining(), 0U);
}

TEST(UbxPayloadWriter, EncodeMessageBuildsValidFrame)
{
    layout_message in{};
    in.kind = wire_enum::first;
    in.value = 1.0;
    ASSERT_EQ(in.label.assign("WXYZ"), castle::status::ok); // full CH[4], no NUL padding

    uint8_t frame[64U] = {};
    castle::size_type written = 0xDEADU;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(20U));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_SYNC_1], UBX_SYNC_CHAR_1);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_SYNC_2], UBX_SYNC_CHAR_2);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], layout_message::msg_class);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], layout_message::msg_id);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), 20U);

    message_view view;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), view));
    EXPECT_TRUE(view.is(layout_message::msg_class, layout_message::msg_id));
    EXPECT_EQ(view.payload_length(), 20U);
    EXPECT_EQ(view.payload[19U], static_cast<uint8_t>('Z'));

    // Exactly-sized buffer also succeeds.
    uint8_t exact[frame_size(20U)] = {};
    ASSERT_EQ(encode_message(in, exact, sizeof(exact), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(exact));
}

TEST(UbxPayloadWriter, EncodeMessageRejectsBadOutputs)
{
    layout_message in{};
    castle::size_type written = 0xDEADU;

    EXPECT_EQ(encode_message(in, nullptr, 64U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);

    uint8_t too_small_for_overhead[UBX_FRAME_OVERHEAD - 1U] = {};
    written = 0xDEADU;
    EXPECT_EQ(encode_message(in, too_small_for_overhead, sizeof(too_small_for_overhead), written),
              castle::status::full);
    EXPECT_EQ(written, 0U);

    uint8_t one_short[frame_size(20U) - 1U] = {};
    written = 0xDEADU;
    EXPECT_EQ(encode_message(in, one_short, sizeof(one_short), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxPayloadWriter, EncodeMessageForwardsEncoderFailure)
{
    uint8_t frame[16U] = {};
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(encode_message(failing_message{}, frame, sizeof(frame), written),
              castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(UbxPayloadWriter, EncodeMessageCapsPayloadAtUbxLimit)
{
    static uint8_t frame[static_cast<castle::size_type>(UBX_MAX_PAYLOAD_LEN) + UBX_FRAME_OVERHEAD + 16U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(encode_message(oversized_message{}, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxPayloadWriter, EncodePollWritesEmptyPayloadFrame)
{
    uint8_t frame[UBX_FRAME_OVERHEAD] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_poll<layout_message>(frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD);

    message_view view;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), view));
    EXPECT_TRUE(view.is(layout_message::msg_class, layout_message::msg_id));
    EXPECT_EQ(view.payload_length(), 0U);

    uint8_t too_small[UBX_FRAME_OVERHEAD - 1U] = {};
    EXPECT_EQ(encode_poll<layout_message>(too_small, sizeof(too_small), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace
