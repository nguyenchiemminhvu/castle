#include "sample_support.hpp"

#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"
#include "castle_ext/parsers/ubx_parser/decoder_registry.hpp"
#include "castle_ext/protocols/ubx/messages/nav_pvt.hpp"
#include "castle_ext/protocols/ubx/messages/nav_status.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"

#include <stdio.h>

// This sample shows the decoder_registry fan-out path: one parser callback
// dispatches each validated frame to every registered message decoder, which
// decodes (if it matches) and notifies its own subscribers. No per-message
// branching is needed at the call site.

using namespace castle_ext::protocols::ubx;
using namespace castle_ext::protocols::ubx::messages;

namespace
{

// Builds a 92-byte UBX-NAV-PVT payload describing a stationary 3D fix.
castle::size_type build_nav_pvt_payload(uint8_t* out)
{
    castle::size_type pos = 0U;
    write_le32(&out[pos], 123456789U); pos += 4U;             // iTOW
    write_le16(&out[pos], 2026U); pos += 2U;                   // year
    out[pos++] = 10U;                                          // month
    out[pos++] = 2U;                                           // day
    out[pos++] = 8U;                                           // hour
    out[pos++] = 30U;                                          // min
    out[pos++] = 15U;                                          // sec
    out[pos++] = nav_pvt::VALID_DATE | nav_pvt::VALID_TIME | nav_pvt::VALID_FULLY;
    write_le32(&out[pos], 25U); pos += 4U;                     // tAcc (ns)
    write_le32(&out[pos], 0U); pos += 4U;                      // nano
    out[pos++] = static_cast<uint8_t>(nav_pvt_fix_type::fix_3d);
    out[pos++] = nav_pvt::FLAGS_GNSS_FIX_OK;                   // flags
    out[pos++] = 0U;                                           // flags2
    out[pos++] = 12U;                                          // numSV
    write_le32(&out[pos], 1068850000U); pos += 4U;             // lon: 106.8850000 deg
    write_le32(&out[pos], 104048500U); pos += 4U;              // lat: 10.4048500 deg
    write_le32(&out[pos], 15000U); pos += 4U;                  // height (mm)
    write_le32(&out[pos], 12000U); pos += 4U;                  // hMSL (mm)
    write_le32(&out[pos], 2500U); pos += 4U;                   // hAcc (mm)
    write_le32(&out[pos], 3000U); pos += 4U;                   // vAcc (mm)
    write_le32(&out[pos], 0U); pos += 4U;                      // velN
    write_le32(&out[pos], 0U); pos += 4U;                      // velE
    write_le32(&out[pos], 0U); pos += 4U;                      // velD
    write_le32(&out[pos], 0U); pos += 4U;                      // gSpeed
    write_le32(&out[pos], 0U); pos += 4U;                      // headMot
    write_le32(&out[pos], 100U); pos += 4U;                    // sAcc
    write_le32(&out[pos], 5000000U); pos += 4U;                // headAcc
    write_le16(&out[pos], 120U); pos += 2U;                    // pDOP: 1.20
    out[pos++] = 0U;                                           // flags3
    for (castle::size_type i = 0U; i < 5U; ++i)                // reserved0
    {
        out[pos++] = 0U;
    }
    write_le32(&out[pos], 0U); pos += 4U;                      // headVeh
    write_le16(&out[pos], 0U); pos += 2U;                      // magDec
    write_le16(&out[pos], 0U); pos += 2U;                      // magAcc
    return pos;
}

// Builds a 16-byte UBX-NAV-STATUS payload reporting a settled 3D fix.
castle::size_type build_nav_status_payload(uint8_t* out)
{
    castle::size_type pos = 0U;
    write_le32(&out[pos], 123456789U); pos += 4U;              // iTOW
    out[pos++] = static_cast<uint8_t>(nav_status_gps_fix::fix_3d);
    out[pos++] = nav_status::FLAGS_GPS_FIX_OK | nav_status::FLAGS_WKN_SET | nav_status::FLAGS_TOW_SET;
    out[pos++] = 0U;                                           // fixStat
    out[pos++] = 0U;                                           // flags2
    write_le32(&out[pos], 29000U); pos += 4U;                  // ttff (ms)
    write_le32(&out[pos], 456789U); pos += 4U;                 // msss (ms)
    return pos;
}

// Builds a 20-byte UBX-NAV-TIMEUTC payload with a fully resolved UTC time.
castle::size_type build_nav_timeutc_payload(uint8_t* out)
{
    static CASTLE_CONSTEXPR uint8_t VALID_TOW_WKN_UTC = 0x07U;

    castle::size_type pos = 0U;
    write_le32(&out[pos], 123456789U); pos += 4U;              // iTOW
    write_le32(&out[pos], 30U); pos += 4U;                     // tAcc (ns)
    write_le32(&out[pos], 0U); pos += 4U;                      // nano
    write_le16(&out[pos], 2026U); pos += 2U;                   // year
    out[pos++] = 10U;                                          // month
    out[pos++] = 2U;                                           // day
    out[pos++] = 8U;                                           // hour
    out[pos++] = 30U;                                          // min
    out[pos++] = 15U;                                          // sec
    out[pos++] = VALID_TOW_WKN_UTC;                            // valid
    return pos;
}

// Encodes `payload` as one UBX frame and feeds it into `parser` byte-by-byte.
template <typename Parser>
void send_frame(Parser& parser, uint8_t msg_class, uint8_t msg_id,
                 castle::container::array_view<CASTLE_CONST uint8_t> payload)
{
    uint8_t frame[128U];
    castle::size_type written = 0U;
    CASTLE_CONST castle::status status =
        encode_frame(msg_class, msg_id, payload, frame, sizeof(frame), written);
    CASTLE_SAMPLE_CHECK(castle::succeeded(status));
    parser.feed(frame, written);
}

} // namespace

int main()
{
    using castle_ext::parsers::ubx_parser::nav_pvt_decoder;
    using castle_ext::parsers::ubx_parser::nav_status_decoder;
    using castle_ext::parsers::ubx_parser::nav_timeutc_decoder;

    castle_ext::parsers::ubx_parser::ubx_parser<> parser;
    castle_ext::parsers::ubx_parser::decoder_registry<
        nav_pvt_decoder<>,
        nav_status_decoder<>,
        nav_timeutc_decoder<>> registry;

    // connect() returns an RAII handle that disconnects the slot on
    // destruction, so each connection must be kept alive for as long as the
    // subscription should remain active.
    uint32_t pvt_count = 0U;
    CASTLE_CONST auto pvt_connection = registry.decoder<nav_pvt>().connect(
        [&pvt_count](CASTLE_CONST nav_pvt& pvt)
        {
            ++pvt_count;
            printf("NAV-PVT:     fix=%s lat=%.7f lon=%.7f sats=%u\n",
                   pvt.fix_ok() ? "3D" : "none",
                   pvt.latitude_deg(), pvt.longitude_deg(),
                   static_cast<unsigned>(pvt.num_sv));
            CASTLE_SAMPLE_CHECK(pvt.fix_ok());
        });

    uint32_t status_count = 0U;
    CASTLE_CONST auto status_connection = registry.decoder<nav_status>().connect(
        [&status_count](CASTLE_CONST nav_status& status)
        {
            ++status_count;
            printf("NAV-STATUS:  fix_ok=%d ttff=%u ms\n",
                   static_cast<int>(status.fix_ok()),
                   static_cast<unsigned>(status.ttff));
        });

    uint32_t timeutc_count = 0U;
    CASTLE_CONST auto timeutc_connection = registry.decoder<nav_timeutc>().connect(
        [&timeutc_count](CASTLE_CONST nav_timeutc& utc)
        {
            ++timeutc_count;
            printf("NAV-TIMEUTC: %04u-%02u-%02u %02u:%02u:%02u\n",
                   static_cast<unsigned>(utc.year), static_cast<unsigned>(utc.month),
                   static_cast<unsigned>(utc.day), static_cast<unsigned>(utc.hour),
                   static_cast<unsigned>(utc.min), static_cast<unsigned>(utc.sec));
        });

    // Wire the parser's message callback to dispatch into the registry.
    castle_ext::parsers::ubx_parser::attach(parser, registry);

    uint8_t nav_pvt_payload[nav_pvt::payload_length];
    build_nav_pvt_payload(nav_pvt_payload);
    send_frame(parser, nav_pvt::msg_class, nav_pvt::msg_id,
               castle::container::array_view<CASTLE_CONST uint8_t>(nav_pvt_payload, sizeof(nav_pvt_payload)));

    uint8_t nav_status_payload[nav_status::payload_length];
    build_nav_status_payload(nav_status_payload);
    send_frame(parser, nav_status::msg_class, nav_status::msg_id,
               castle::container::array_view<CASTLE_CONST uint8_t>(nav_status_payload, sizeof(nav_status_payload)));

    uint8_t nav_timeutc_payload[nav_timeutc::payload_length];
    build_nav_timeutc_payload(nav_timeutc_payload);
    send_frame(parser, nav_timeutc::msg_class, nav_timeutc::msg_id,
               castle::container::array_view<CASTLE_CONST uint8_t>(nav_timeutc_payload, sizeof(nav_timeutc_payload)));

    CASTLE_SAMPLE_CHECK(pvt_count == 1U);
    CASTLE_SAMPLE_CHECK(status_count == 1U);
    CASTLE_SAMPLE_CHECK(timeutc_count == 1U);
    CASTLE_SAMPLE_CHECK(parser.frames_decoded() == 3U);

    return 0;
}

