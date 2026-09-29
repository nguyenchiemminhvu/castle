#include "sample_support.hpp"

#include "castle_ext/parsers/nmea_parser/nmea_parser.hpp"

#include <string.h>

int main()
{
    using namespace castle::protocols::nmea;
    using castle::parsers::nmea_parser::parse_error;
    using castle::parsers::nmea_parser::parser_error_code;

    castle::parsers::nmea_parser::nmea_parser<> parser;

    uint32_t messages_received = 0U;
    parser.set_message_callback(
        [&messages_received](const message_view& sentence)
        {
            ++messages_received;
            if (sentence.is(SENTENCE_GGA))
            {
                // Decode sentence.field(i) values here, e.g. with parse_latlon().
            }
        });

    uint32_t errors_received = 0U;
    parser.set_error_callback(
        [&errors_received](const parse_error& error)
        {
            ++errors_received;
            (void)castle::parsers::nmea_parser::error_message(error.code);
        });

    // Feed one well-known, valid GGA sentence byte-by-byte, as a UART driver would.
    static const char gga[] =
        "$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,*5B\r\n";
    for (CASTLE_CONST char* p = gga; *p != '\0'; ++p)
    {
        parser.feed(*p);
    }
    CASTLE_SAMPLE_CHECK(messages_received == 1U);
    CASTLE_SAMPLE_CHECK(parser.messages_decoded() == 1U);

    // Binary noise interleaved in the stream is discarded while resynchronizing.
    uint8_t noise[3U] = {0xB5U, 0x62U, 0x01U};
    parser.feed(noise, sizeof(noise));
    parser.feed(gga, strlen(gga));
    CASTLE_SAMPLE_CHECK(messages_received == 2U);

    // A tampered checksum is reported through the error callback instead.
    static const char bad_checksum[] =
        "$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,*00\r\n";
    parser.feed(bad_checksum, strlen(bad_checksum));
    CASTLE_SAMPLE_CHECK(errors_received == 1U);
    CASTLE_SAMPLE_CHECK(parser.messages_discarded() == 1U);

    // Some proprietary messages omit the checksum entirely; still delivered.
    static const char no_checksum[] = "$GPTXT,1,1,01,ANTSTATUS=OK\r\n";
    parser.feed(no_checksum, strlen(no_checksum));
    CASTLE_SAMPLE_CHECK(messages_received == 3U);

    return 0;
}
