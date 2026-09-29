#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/nmea_generator.hpp"
#include "castle_ext/protocols/nmea/messages/zda.hpp"

int main()
{
    using namespace castle::protocols::nmea;
    using namespace castle::protocols::nmea::generator;
    using namespace castle::protocols::nmea::messages;

    zda_input input;
    input.hour = 9U;
    input.minute = 27U;
    input.second = 25U;
    input.day = 12U;
    input.month = 8U;
    input.year = 2026U;
    input.tz_hour = -5;
    input.tz_min = 30U;
    input.valid = true;

    char sentence[128U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(
        generate_zda(input, castle::container::string_view(TALKER_GP), sentence, sizeof(sentence), written)));

    castle::container::string_view field_storage[8U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(
        castle::container::string_view(sentence, written), field_storage, 8U, raw));

    CASTLE_SAMPLE_CHECK(zda::matches(raw));

    zda fix;
    CASTLE_SAMPLE_CHECK(zda::decode(raw, fix));
    CASTLE_SAMPLE_CHECK(fix.valid);
    CASTLE_SAMPLE_CHECK(fix.day == 12U);
    CASTLE_SAMPLE_CHECK(fix.month == 8U);
    CASTLE_SAMPLE_CHECK(fix.year == 2026U);
    CASTLE_SAMPLE_CHECK(fix.tz_hour == -5);
    CASTLE_SAMPLE_CHECK(fix.tz_min == 30U);

    return 0;
}
