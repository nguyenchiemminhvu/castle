#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/messages/gns.hpp"

int main()
{
    using namespace castle::protocols::nmea;
    using namespace castle::protocols::nmea::messages;

    // No generator exists for GNS; the checksum suffix is simply omitted.
    CASTLE_CONST castle::container::string_view literal(
        "$GPGNS,014035.00,4332.69262,S,17235.48549,E,RR,13,0.9,25.63,11.24,,\r\n");

    castle::container::string_view field_storage[16U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(literal, field_storage, 16U, raw));

    CASTLE_SAMPLE_CHECK(gns::matches(raw));

    gns fix;
    CASTLE_SAMPLE_CHECK(gns::decode(raw, fix));
    CASTLE_SAMPLE_CHECK(fix.valid);
    CASTLE_SAMPLE_CHECK(fix.mode_indicator == castle::container::string_view("RR"));
    CASTLE_SAMPLE_CHECK((fix.latitude < -43.0) && (fix.latitude > -44.0)); // 'S' -> negative
    CASTLE_SAMPLE_CHECK((fix.longitude > 172.0) && (fix.longitude < 173.0));
    CASTLE_SAMPLE_CHECK(fix.num_sats == 13);
    CASTLE_SAMPLE_CHECK((fix.hdop > 0.89) && (fix.hdop < 0.91));
    CASTLE_SAMPLE_CHECK((fix.altitude > 25.0) && (fix.altitude < 26.0));
    CASTLE_SAMPLE_CHECK((fix.geoid_sep > 11.0) && (fix.geoid_sep < 12.0));
    // Diff age/ref were left empty on the wire; defaults are kept.
    CASTLE_SAMPLE_CHECK(fix.diff_age < 0.0);
    CASTLE_SAMPLE_CHECK(fix.diff_ref == -1);

    return 0;
}
