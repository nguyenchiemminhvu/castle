#include "sample_support.hpp"

#include "castle_ext/protocols/rtcm/bit_writer.hpp"
#include "castle_ext/protocols/rtcm/messages/station_arp.hpp"

namespace
{

/** @brief Minimal message model: 12-bit number, signed and unsigned fields, flag, text. */
struct sample_message
{
    uint16_t number = 0U;
    int32_t offset = 0;
    bool flag = false;
    castle::container::string<8U> name{};

    void encode(castle::protocols::rtcm::bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.field(number, 12U);
        writer.field(offset, 12U);
        writer.field(flag);
        writer.reserved(3U);
        writer.text(name);
    }
};

} // namespace

int main()
{
    using namespace castle::protocols::rtcm;
    using castle::container::array_view;

    // 1. Raw bits, MSB first, no alignment between fields.
    uint8_t payload[8U];
    for (uint8_t& byte : payload)
    {
        byte = 0xFFU; // pad bits are zeroed even over dirty storage
    }
    bit_writer writer(array_view<uint8_t>(payload, sizeof(payload)));
    writer.write_bits(0x5U, 3U);   // 101
    writer.write_bits(0xFFU, 8U);  // 11111111
    writer.write_bits(0x1U, 1U);   // 1
    CASTLE_SAMPLE_CHECK(writer.ok());
    CASTLE_SAMPLE_CHECK(writer.status() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(writer.bit_size() == 12U);
    CASTLE_SAMPLE_CHECK(writer.size() == 2U);
    CASTLE_SAMPLE_CHECK(writer.remaining_bits() == 52U);
    CASTLE_SAMPLE_CHECK(payload[0U] == 0xBFU);
    CASTLE_SAMPLE_CHECK(payload[1U] == 0xF0U);
    CASTLE_SAMPLE_CHECK(payload[2U] == 0xFFU); // beyond the payload, untouched

    // 2. Typed data fields: unsigned, two's complement signed, flag, reserved bits, text.
    uint8_t fields[16U] = {};
    bit_writer typed(array_view<uint8_t>(fields, sizeof(fields)));
    typed.field(static_cast<uint16_t>(1005U), 12U);
    typed.field(static_cast<int32_t>(-2), 12U);
    typed.field(true);
    typed.reserved(3U);
    castle::container::string<8U> label;
    CASTLE_SAMPLE_CHECK(label.assign("AB") == castle::status::ok);
    typed.text(label);
    CASTLE_SAMPLE_CHECK(typed.ok());
    CASTLE_SAMPLE_CHECK(typed.bit_size() == 12U + 12U + 1U + 3U + 24U);
    CASTLE_SAMPLE_CHECK(fields[0U] == 0x3EU);
    CASTLE_SAMPLE_CHECK(fields[1U] == 0xDFU);
    CASTLE_SAMPLE_CHECK(fields[2U] == 0xFEU);
    CASTLE_SAMPLE_CHECK(fields[3U] == 0x80U); // flag + 3 reserved bits + start of the count
    CASTLE_SAMPLE_CHECK(fields[4U] == 0x24U); // 0x02 count, then 'A' = 0x41 straddling the nibble
    CASTLE_SAMPLE_CHECK(fields[5U] == 0x14U);

    // 3. Sticky errors: out-of-range values and overflow latch a status, later writes are ignored.
    uint8_t tiny[1U] = {};
    bit_writer failing(array_view<uint8_t>(tiny, sizeof(tiny)));
    failing.field(static_cast<uint8_t>(0x10U), 4U); // does not fit 4 bits
    CASTLE_SAMPLE_CHECK(!failing.ok());
    CASTLE_SAMPLE_CHECK(failing.status() == castle::status::out_of_range);
    failing.write_bits(0U, 16U); // would overflow, but the first failure is kept
    failing.fail(castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(failing.status() == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(failing.bit_size() == 0U);

    bit_writer overflow(array_view<uint8_t>(tiny, sizeof(tiny)));
    overflow.write_bits(0U, 9U);
    CASTLE_SAMPLE_CHECK(overflow.status() == castle::status::full);

    // A writer over a null view has zero capacity.
    bit_writer none{array_view<uint8_t>{}};
    CASTLE_SAMPLE_CHECK(none.remaining_bits() == 0U);
    none.field(true);
    CASTLE_SAMPLE_CHECK(none.status() == castle::status::full);

    // 4. encode_message(): the model writes the payload straight into the frame buffer.
    sample_message message{};
    message.number = 0xABCU;
    message.offset = -2;
    message.flag = true;
    CASTLE_SAMPLE_CHECK(message.name.assign("F9R") == castle::status::ok);

    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    castle::status result = encode_message(message, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    CASTLE_SAMPLE_CHECK(written == frame_size(8U));
    CASTLE_SAMPLE_CHECK(frame[0U] == RTCM_PREAMBLE);
    CASTLE_SAMPLE_CHECK(castle::checksums::crc24q::calculate(frame, written) == 0U);

    uint8_t tiny_frame[8U] = {};
    result = encode_message(message, tiny_frame, sizeof(tiny_frame), written);
    CASTLE_SAMPLE_CHECK(result == castle::status::full);
    CASTLE_SAMPLE_CHECK(written == 0U);
    result = encode_message(message, nullptr, 32U, written);
    CASTLE_SAMPLE_CHECK(result == castle::status::invalid_argument);

    // 5. A real message: station_arp (message 1005) produces the pyrtcm-verified frame.
    messages::station_arp station{};
    station.station_id = 1234U;
    station.itrf_year = 14U;
    station.gps = true;
    station.glonass = true;
    station.single_receiver_oscillator = true;
    station.quarter_cycle_indicator = 2U;
    station.set_position_m(4027894.0060, 307045.6000, 4919474.9100);
    result = encode_message(station, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(result));
    CASTLE_SAMPLE_CHECK(written == 25U);
    CASTLE_SAMPLE_CHECK(frame[written - 3U] == 0x19U);
    CASTLE_SAMPLE_CHECK(frame[written - 2U] == 0xD1U);
    CASTLE_SAMPLE_CHECK(frame[written - 1U] == 0xF1U);

    return 0;
}
