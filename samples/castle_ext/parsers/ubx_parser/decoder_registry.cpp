#include "sample_support.hpp"

#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"
#include "castle_ext/parsers/ubx_parser/decoder_registry.hpp"
#include "castle_ext/protocols/ubx/messages/nav_pvt.hpp"
#include "castle_ext/protocols/ubx/messages/nav_status.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"

// This sample shows the decoder_registry fan-out path: one parser callback
// dispatches each validated frame to every registered message decoder, which
// decodes (if it matches) and notifies its own subscribers. No per-message
// branching is needed at the call site.

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

namespace
{

/** @brief Encodes `message` as one UBX frame and feeds it into `parser`. */
template <typename Parser, typename Message>
void send_message(Parser& parser, CASTLE_CONST Message& message)
{
    uint8_t frame[128U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(message, frame, sizeof(frame), written)));
    parser.feed(frame, written);
}

} // namespace

int main()
{
    using castle::parsers::ubx_parser::nav_pvt_decoder;
    using castle::parsers::ubx_parser::nav_status_decoder;
    using castle::parsers::ubx_parser::nav_timeutc_decoder;

    castle::parsers::ubx_parser::ubx_parser<> parser;
    castle::parsers::ubx_parser::decoder_registry<
        nav_pvt_decoder<>,
        nav_status_decoder<>,
        nav_timeutc_decoder<>> registry;

    // connect() returns an RAII handle that disconnects the slot on
    // destruction, so each connection must be kept alive for as long as the
    // subscription should remain active.
    uint32_t pvt_count = 0U;
    nav_pvt last_pvt{};
    CASTLE_CONST auto pvt_connection = registry.decoder<nav_pvt>().connect(
        [&pvt_count, &last_pvt](CASTLE_CONST nav_pvt& pvt)
        {
            ++pvt_count;
            last_pvt = pvt;
        });

    uint32_t status_count = 0U;
    uint32_t last_ttff = 0U;
    CASTLE_CONST auto status_connection = registry.decoder<nav_status>().connect(
        [&status_count, &last_ttff](CASTLE_CONST nav_status& status)
        {
            ++status_count;
            CASTLE_SAMPLE_CHECK(status.fix_ok());
            last_ttff = status.ttff;
        });

    uint32_t timeutc_count = 0U;
    nav_timeutc last_utc{};
    CASTLE_CONST auto timeutc_connection = registry.decoder<nav_timeutc>().connect(
        [&timeutc_count, &last_utc](CASTLE_CONST nav_timeutc& utc)
        {
            ++timeutc_count;
            last_utc = utc;
        });

    // Wire the parser's message callback to dispatch into the registry.
    castle::parsers::ubx_parser::attach(parser, registry);

    // A stationary 3D fix.
    nav_pvt pvt{};
    pvt.i_tow = 123456789U;
    pvt.year = 2026U;
    pvt.month = 10U;
    pvt.day = 2U;
    pvt.hour = 8U;
    pvt.minute = 30U;
    pvt.second = 15U;
    pvt.valid = nav_pvt::VALID_DATE | nav_pvt::VALID_TIME | nav_pvt::VALID_FULLY;
    pvt.fix_type = nav_pvt_fix_type::fix_3d;
    pvt.flags = nav_pvt::FLAGS_GNSS_FIX_OK;
    pvt.num_sv = 12U;
    pvt.lon = 1068850000;   // 106.8850000 deg
    pvt.lat = 104048500;    // 10.4048500 deg
    pvt.p_dop = 120U;       // 1.20
    send_message(parser, pvt);

    // A settled navigation status.
    nav_status status{};
    status.i_tow = 123456789U;
    status.gps_fix = nav_status_gps_fix::fix_3d;
    status.flags = nav_status::FLAGS_GPS_FIX_OK | nav_status::FLAGS_WKN_SET | nav_status::FLAGS_TOW_SET;
    status.ttff = 29000U;
    send_message(parser, status);

    // A fully resolved UTC time.
    nav_timeutc utc{};
    utc.i_tow = 123456789U;
    utc.year = 2026U;
    utc.month = 10U;
    utc.day = 2U;
    utc.hour = 8U;
    utc.min = 30U;
    utc.sec = 15U;
    utc.valid = nav_timeutc::VALID_TOW | nav_timeutc::VALID_WKN | nav_timeutc::VALID_UTC;
    send_message(parser, utc);

    CASTLE_SAMPLE_CHECK(pvt_count == 1U);
    CASTLE_SAMPLE_CHECK(last_pvt.fix_ok());
    CASTLE_SAMPLE_CHECK(last_pvt.num_sv == 12U);
    CASTLE_SAMPLE_CHECK(last_pvt.lat == 104048500);
    CASTLE_SAMPLE_CHECK(last_pvt.latitude_deg() > 10.40484 && last_pvt.latitude_deg() < 10.40486);

    CASTLE_SAMPLE_CHECK(status_count == 1U);
    CASTLE_SAMPLE_CHECK(last_ttff == 29000U);

    CASTLE_SAMPLE_CHECK(timeutc_count == 1U);
    CASTLE_SAMPLE_CHECK(last_utc.year == 2026U);
    CASTLE_SAMPLE_CHECK(last_utc.sec == 15U);

    CASTLE_SAMPLE_CHECK(parser.frames_decoded() == 3U);

    return 0;
}
