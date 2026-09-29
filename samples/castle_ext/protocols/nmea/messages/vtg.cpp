#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/nmea_generator.hpp"
#include "castle_ext/protocols/nmea/messages/vtg.hpp"

int main()
{
    using namespace castle::protocols::nmea;
    using namespace castle::protocols::nmea::generator;
    using namespace castle::protocols::nmea::messages;

    vtg_input input;
    input.course_true = 182.5;
    input.course_mag = 180.1;
    input.course_mag_valid = true;
    input.speed_knots = 3.25;
    input.speed_kmh = 6.02;
    input.mode = 'A';
    input.valid = true;

    char sentence[128U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(
        generate_vtg(input, castle::container::string_view(TALKER_GP), sentence, sizeof(sentence), written)));

    castle::container::string_view field_storage[12U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(
        castle::container::string_view(sentence, written), field_storage, 12U, raw));

    CASTLE_SAMPLE_CHECK(vtg::matches(raw));

    vtg fix;
    CASTLE_SAMPLE_CHECK(vtg::decode(raw, fix));
    CASTLE_SAMPLE_CHECK(fix.valid);
    CASTLE_SAMPLE_CHECK((fix.course_true > 182.0) && (fix.course_true < 183.0));
    CASTLE_SAMPLE_CHECK(fix.course_mag_valid);
    CASTLE_SAMPLE_CHECK((fix.course_mag > 180.0) && (fix.course_mag < 180.2));
    CASTLE_SAMPLE_CHECK((fix.speed_knots > 3.2) && (fix.speed_knots < 3.3));
    CASTLE_SAMPLE_CHECK((fix.speed_kmh > 6.0) && (fix.speed_kmh < 6.1));
    CASTLE_SAMPLE_CHECK(fix.mode == 'A');

    return 0;
}
