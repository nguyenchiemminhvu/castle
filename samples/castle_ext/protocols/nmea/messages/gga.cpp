#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/nmea_generator.hpp"
#include "castle_ext/protocols/nmea/messages/gga.hpp"

int main()
{
    using namespace castle_ext::protocols::nmea;
    using namespace castle_ext::protocols::nmea::generator;
    using namespace castle_ext::protocols::nmea::messages;

    gga_input input;
    input.hour = 9U;
    input.minute = 27U;
    input.second = 25U;
    input.latitude_deg = 47.2852333;
    input.longitude_deg = 8.5652650;
    input.fix_quality = 1U;
    input.num_satellites = 8U;
    input.hdop = 1.01;
    input.altitude_msl_m = 499.6;
    input.geoid_sep_m = 48.0;
    input.valid = true;

    char sentence[128U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(
        generate_gga(input, castle::container::string_view(TALKER_GP), sentence, sizeof(sentence), written)));

    castle::container::string_view field_storage[16U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(
        castle::container::string_view(sentence, written), field_storage, 16U, raw));

    // Dispatch-style check before committing to the typed decode.
    CASTLE_SAMPLE_CHECK(gga::matches(raw));

    gga fix;
    CASTLE_SAMPLE_CHECK(gga::decode(raw, fix));
    CASTLE_SAMPLE_CHECK(fix.valid);
    CASTLE_SAMPLE_CHECK(fix.fix_available());
    CASTLE_SAMPLE_CHECK(fix.fix_quality == gga_fix_quality::gps_sps);
    CASTLE_SAMPLE_CHECK(fix.num_satellites == 8U);
    CASTLE_SAMPLE_CHECK((fix.latitude > 47.28) && (fix.latitude < 47.29));
    CASTLE_SAMPLE_CHECK((fix.longitude > 8.56) && (fix.longitude < 8.57));
    CASTLE_SAMPLE_CHECK((fix.altitude_msl > 499.0) && (fix.altitude_msl < 500.0));
    CASTLE_SAMPLE_CHECK((fix.hdop > 0.99) && (fix.hdop < 1.01));

    // A sentence of a different type never matches.
    castle::container::string_view other_fields[4U];
    message_view other_raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(
        castle::container::string_view("$GPVTG,182.5,T,,M,3.25,N,6.02,K,A*XX\r\n"), other_fields, 4U, other_raw) == false);
    CASTLE_SAMPLE_CHECK(!gga::matches(other_raw));

    return 0;
}
