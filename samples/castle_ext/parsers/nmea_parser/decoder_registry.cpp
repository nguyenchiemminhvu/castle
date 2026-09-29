#include "sample_support.hpp"

#include "castle_ext/parsers/nmea_parser/nmea_parser.hpp"
#include "castle_ext/parsers/nmea_parser/decoder_registry.hpp"
#include "castle_ext/protocols/nmea/messages/dtm.hpp"
#include "castle_ext/protocols/nmea/messages/gga.hpp"
#include "castle_ext/protocols/nmea/messages/rmc.hpp"
#include "castle_ext/protocols/nmea/messages/gsa.hpp"
#include "castle_ext/protocols/nmea/messages/vtg.hpp"

#include <stdio.h>
#include <string.h>

// This sample shows the decoder_registry fan-out path: one parser callback
// dispatches each validated sentence to every registered sentence decoder,
// which decodes (if it matches) and notifies its own subscribers. No
// per-sentence branching is needed at the call site.

using namespace castle::protocols::nmea::messages;

int main()
{
    using castle::parsers::nmea_parser::dtm_decoder;
    using castle::parsers::nmea_parser::gga_decoder;
    using castle::parsers::nmea_parser::rmc_decoder;
    using castle::parsers::nmea_parser::gsa_decoder;
    using castle::parsers::nmea_parser::vtg_decoder;

    castle::parsers::nmea_parser::nmea_parser<> parser;
    castle::parsers::nmea_parser::decoder_registry<
        dtm_decoder<>,
        gga_decoder<>,
        rmc_decoder<>,
        gsa_decoder<>,
        vtg_decoder<>> registry;

    // connect() returns an RAII handle that disconnects the slot on
    // destruction, so each connection must be kept alive for as long as the
    // subscription should remain active.
    uint32_t gga_count = 0U;
    CASTLE_CONST auto gga_connection = registry.decoder<gga>().connect(
        [&gga_count](CASTLE_CONST gga& fix)
        {
            ++gga_count;
            printf("GGA: lat=%.6f lon=%.6f sats=%u hdop=%.2f\n",
                   fix.latitude, fix.longitude,
                   static_cast<unsigned>(fix.num_satellites), fix.hdop);
            CASTLE_SAMPLE_CHECK(fix.fix_available());
        });

    uint32_t rmc_count = 0U;
    CASTLE_CONST auto rmc_connection = registry.decoder<rmc>().connect(
        [&rmc_count](CASTLE_CONST rmc& minimum)
        {
            ++rmc_count;
            printf("RMC: speed=%.3f kn course=%.2f deg date=%06u\n",
                   minimum.speed_knots, minimum.course_true,
                   static_cast<unsigned>(minimum.date));
            CASTLE_SAMPLE_CHECK(minimum.status_active);
        });

    uint32_t gsa_count = 0U;
    CASTLE_CONST auto gsa_connection = registry.decoder<gsa>().connect(
        [&gsa_count](CASTLE_CONST gsa& dop)
        {
            ++gsa_count;
            printf("GSA: nav_mode=%u pdop=%.2f hdop=%.2f vdop=%.2f\n",
                   static_cast<unsigned>(dop.nav_mode), dop.pdop, dop.hdop, dop.vdop);
        });

    uint32_t vtg_count = 0U;
    CASTLE_CONST auto vtg_connection = registry.decoder<vtg>().connect(
        [&vtg_count](CASTLE_CONST vtg& track)
        {
            ++vtg_count;
            printf("VTG: course=%.2f deg speed=%.3f km/h\n", track.course_true, track.speed_kmh);
        });

    uint32_t dtm_count = 0U;
    CASTLE_CONST auto dtm_connection = registry.decoder<dtm>().connect(
        [&dtm_count](CASTLE_CONST dtm& datum)
        {
            ++dtm_count;
            printf("DTM: datum=%.*s lat_off=%.3f lon_off=%.3f\n",
                   static_cast<int>(datum.datum_code.size()), datum.datum_code.data(),
                   datum.lat_offset, datum.lon_offset);
        });

    // Wire the parser's message callback to dispatch into the registry.
    castle::parsers::nmea_parser::attach(parser, registry);

    // Feed a representative stream of messages, as a GNSS receiver UART would.
    static CASTLE_CONST char* CASTLE_CONST stream[] = {
        "$GPGGA,092725.00,4717.11399,N,00833.91590,E,1,08,1.01,499.6,M,48.0,M,,*5B\r\n",
        "$GPRMC,092725.00,A,4717.11399,N,00833.91590,E,0.004,77.52,091202,,,A*54\r\n",
        "$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39\r\n",
        "$GPVTG,77.52,T,,M,0.004,N,0.008,K,A*06\r\n",
        "$GPDTM,W84,,0.0,N,0.0,E,0.0,W84*6F\r\n"
    };

    for (CASTLE_CONST char* sentence : stream)
    {
        parser.feed(sentence, strlen(sentence));
    }

    CASTLE_SAMPLE_CHECK(gga_count == 1U);
    CASTLE_SAMPLE_CHECK(rmc_count == 1U);
    CASTLE_SAMPLE_CHECK(gsa_count == 1U);
    CASTLE_SAMPLE_CHECK(vtg_count == 1U);
    CASTLE_SAMPLE_CHECK(dtm_count == 1U);
    CASTLE_SAMPLE_CHECK(parser.messages_decoded() == 5U);

    return 0;
}

