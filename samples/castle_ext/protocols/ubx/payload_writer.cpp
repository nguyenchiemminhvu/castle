#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"

namespace
{

enum class sample_mode : uint8_t
{
    idle = 0U,
    active = 2U
};

/** @brief Minimal message model: one layout serves encode and decode. */
struct sample_message
{
    static CASTLE_CONSTEXPR uint8_t msg_class = castle::protocols::ubx::UBX_CLASS_CFG;
    static CASTLE_CONSTEXPR uint8_t msg_id = castle::protocols::ubx::UBX_ID_CFG_SPT;

    sample_mode mode = sample_mode::idle;
    int16_t offset = 0;
    castle::container::array<uint8_t, 2U> tags{};
    castle::container::string<6U> name{};

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(0x01U);
        stream.field(self.mode);
        stream.field(self.offset);
        stream.field(self.tags);
        stream.reserved(1U);
        stream.text(self.name);
    }

    void encode(castle::protocols::ubx::payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        layout(*this, writer);
    }
};

} // namespace

int main()
{
    using namespace castle::protocols::ubx;
    using castle::container::array_view;

    // 1. Raw writer: little-endian fields, reserved bytes, typed writes.
    uint8_t payload[16U] = {};
    payload_writer writer(array_view<uint8_t>(payload, sizeof(payload)));
    writer.write(static_cast<uint16_t>(0x1234U));
    writer.write(static_cast<int32_t>(-2));
    writer.write(1.5F);
    writer.write_zeros(2U);
    writer.write_uint(0xAABBCCU, 3U);
    CASTLE_CONST uint8_t raw_bytes[1U] = {0x7EU};
    writer.write_bytes(raw_bytes, 1U);

    CASTLE_SAMPLE_CHECK(writer.ok());
    CASTLE_SAMPLE_CHECK(writer.status() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(writer.size() == 16U);
    CASTLE_SAMPLE_CHECK(writer.remaining() == 0U);
    CASTLE_SAMPLE_CHECK(payload[0U] == 0x34U);
    CASTLE_SAMPLE_CHECK(payload[1U] == 0x12U);
    CASTLE_SAMPLE_CHECK(payload[12U] == 0xCCU);
    CASTLE_SAMPLE_CHECK(payload[15U] == 0x7EU);

    // The reader mirrors the writer.
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    CASTLE_SAMPLE_CHECK(reader.read_u16() == 0x1234U);
    CASTLE_SAMPLE_CHECK(reader.read_i32() == -2);
    CASTLE_SAMPLE_CHECK(reader.read_r4() == 1.5F);

    // 2. Overflow latches status::full and later writes are ignored.
    writer.write(static_cast<uint8_t>(1U));
    CASTLE_SAMPLE_CHECK(!writer.ok());
    CASTLE_SAMPLE_CHECK(writer.status() == castle::status::full);
    CASTLE_SAMPLE_CHECK(writer.size() == 16U);

    // 3. Explicit encoder failure keeps the first reason.
    uint8_t scratch[4U] = {};
    payload_writer failing(array_view<uint8_t>(scratch, sizeof(scratch)));
    failing.fail(castle::status::invalid_argument);
    failing.fail(castle::status::full);
    CASTLE_SAMPLE_CHECK(failing.status() == castle::status::invalid_argument);

    // 4. encode_message: one layout drives the whole frame, no scratch copy.
    sample_message message{};
    message.mode = sample_mode::active;
    message.offset = -300;
    message.tags[0U] = 0xA1U;
    message.tags[1U] = 0xB2U;
    static_cast<void>(message.name.assign("F9R"));

    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    castle::status result = encode_message(message, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    CASTLE_SAMPLE_CHECK(written == frame_size(13U));

    message_view view;
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), view));
    CASTLE_SAMPLE_CHECK(view.is(sample_message::msg_class, sample_message::msg_id));

    sample_message decoded{};
    payload_reader frame_reader(view.payload);
    sample_message::layout(decoded, frame_reader);
    CASTLE_SAMPLE_CHECK(frame_reader.ok());
    CASTLE_SAMPLE_CHECK(decoded.mode == sample_mode::active);
    CASTLE_SAMPLE_CHECK(decoded.offset == -300);
    CASTLE_SAMPLE_CHECK(decoded.tags[1U] == 0xB2U);
    CASTLE_SAMPLE_CHECK(decoded.name.view() == message.name.view());

    // A buffer that cannot hold the frame reports full and writes nothing.
    uint8_t tiny[12U] = {};
    result = encode_message(message, tiny, sizeof(tiny), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::full);
    CASTLE_SAMPLE_CHECK(written == 0U);

    // 5. encode_poll: empty-payload poll request.
    result = encode_poll<sample_message>(frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    CASTLE_SAMPLE_CHECK(written == UBX_FRAME_OVERHEAD);
    CASTLE_SAMPLE_CHECK(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), view));
    CASTLE_SAMPLE_CHECK(view.payload_length() == 0U);

    return 0;
}
