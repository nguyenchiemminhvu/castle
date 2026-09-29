#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/nmea_generator.hpp"
#include "castle_ext/protocols/nmea/messages/rmc.hpp"

int main()
{
    using namespace castle_ext::protocols::nmea;
    using namespace castle_ext::protocols::nmea::generator;
    using namespace castle_ext::protocols::nmea::messages;

    rmc_input input;
    input.hour = 9U;
    input.minute = 27U;
    input.second = 25U;
    input.status_active = true;
    input.latitude_deg = 47.2852333;
    input.longitude_deg = 8.5652650;
    input.speed_knots = 3.25;
    input.course_true = 182.5;
    input.day = 12U;
    input.month = 8U;
    input.year = 2026U;
    input.mag_variation = -1.5; // negative -> 'W' direction on the wire
    input.mag_var_valid = true;
    input.mode = 'A';
    input.valid = true;

    char sentence[128U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(
        generate_rmc(input, castle::container::string_view(TALKER_GP), sentence, sizeof(sentence), written)));

    castle::container::string_view field_storage[16U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(
        castle::container::string_view(sentence, written), field_storage, 16U, raw));

    CASTLE_SAMPLE_CHECK(rmc::matches(raw));

    rmc fix;
    CASTLE_SAMPLE_CHECK(rmc::decode(raw, fix));
    CASTLE_SAMPLE_CHECK(fix.valid);
    CASTLE_SAMPLE_CHECK(fix.status_active);
    CASTLE_SAMPLE_CHECK((fix.latitude > 47.28) && (fix.latitude < 47.29));
    CASTLE_SAMPLE_CHECK((fix.longitude > 8.56) && (fix.longitude < 8.57));
    CASTLE_SAMPLE_CHECK((fix.speed_knots > 3.2) && (fix.speed_knots < 3.3));
    CASTLE_SAMPLE_CHECK((fix.course_true > 182.0) && (fix.course_true < 183.0));
    CASTLE_SAMPLE_CHECK(fix.date == 120826U);
    // The sign-restoring 'W' direction byte round-trips back to a negative value.
    CASTLE_SAMPLE_CHECK((fix.mag_variation > -1.6) && (fix.mag_variation < -1.4));
    CASTLE_SAMPLE_CHECK(fix.mode == rmc_mode::autonomous);

    return 0;
}
