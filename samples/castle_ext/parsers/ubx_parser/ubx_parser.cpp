#include "sample_support.hpp"

#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"

int main()
{
    using namespace castle_ext::protocols::ubx;
    using castle_ext::parsers::ubx_parser::parse_error;
    using castle_ext::parsers::ubx_parser::parser_error_code;

    castle_ext::parsers::ubx_parser::ubx_parser<> parser;

    uint32_t messages_received = 0U;
    parser.set_message_callback(
        [&messages_received](const message_view& message)
        {
            ++messages_received;
            if (message.is(UBX_CLASS_MON, UBX_ID_MON_VER))
            {
                // Decode message.payload fields here, e.g. with read_le32().
            }
        });

    uint32_t errors_received = 0U;
    parser.set_error_callback(
        [&errors_received](const parse_error& error)
        {
            ++errors_received;
            (void)castle_ext::parsers::ubx_parser::error_message(error.code);
        });

    // Build one valid frame and feed it byte-by-byte, as a UART driver would.
    uint8_t payload_data[2U] = {0xAAU, 0xBBU};
    castle::container::array_view<const uint8_t> payload(payload_data, 2U);

    uint8_t frame[64U];
    castle::size_type written = 0U;
    castle::status encode_status = encode_frame(
        UBX_CLASS_MON, UBX_ID_MON_VER, payload, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_status));

    for (castle::size_type i = 0U; i < written; ++i)
    {
        parser.feed(frame[i]);
    }
    CASTLE_SAMPLE_CHECK(messages_received == 1U);
    CASTLE_SAMPLE_CHECK(parser.frames_decoded() == 1U);

    // Noise bytes (e.g. interleaved NMEA text) are discarded while resynchronizing.
    uint8_t noise[3U] = {'$', 'G', 'P'};
    parser.feed(noise, sizeof(noise));
    parser.feed(frame, written);
    CASTLE_SAMPLE_CHECK(messages_received == 2U);

    // A corrupted checksum byte is reported through the error callback instead.
    frame[written - 1U] ^= 0xFFU;
    parser.feed(frame, written);
    CASTLE_SAMPLE_CHECK(errors_received == 1U);
    CASTLE_SAMPLE_CHECK(parser.frames_discarded() == 1U);

    return 0;
}
