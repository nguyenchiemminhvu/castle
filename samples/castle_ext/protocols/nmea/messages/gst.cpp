#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/messages/gst.hpp"

int main()
{
    using namespace castle::protocols::nmea;
    using namespace castle::protocols::nmea::messages;

    // No generator exists for GST; the checksum suffix is simply omitted.
    CASTLE_CONST castle::container::string_view literal(
        "$GPGST,024603.00,3.1,0.9,0.5,180.0,0.8,0.6,1.2\r\n");

    castle::container::string_view field_storage[16U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(literal, field_storage, 16U, raw));

    CASTLE_SAMPLE_CHECK(gst::matches(raw));

    gst stats;
    CASTLE_SAMPLE_CHECK(gst::decode(raw, stats));
    CASTLE_SAMPLE_CHECK(stats.valid);
    CASTLE_SAMPLE_CHECK((stats.rms_dev > 3.0) && (stats.rms_dev < 3.2));
    CASTLE_SAMPLE_CHECK((stats.semi_major > 0.8) && (stats.semi_major < 1.0));
    CASTLE_SAMPLE_CHECK((stats.semi_minor > 0.4) && (stats.semi_minor < 0.6));
    CASTLE_SAMPLE_CHECK((stats.orient > 179.0) && (stats.orient < 181.0));
    CASTLE_SAMPLE_CHECK((stats.lat_err > 0.7) && (stats.lat_err < 0.9));
    CASTLE_SAMPLE_CHECK((stats.lon_err > 0.5) && (stats.lon_err < 0.7));
    CASTLE_SAMPLE_CHECK((stats.alt_err > 1.1) && (stats.alt_err < 1.3));

    return 0;
}
