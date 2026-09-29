#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/nmea_generator.hpp"
#include "castle_ext/protocols/nmea/messages/gll.hpp"

int main()
{
    using namespace castle_ext::protocols::nmea;
    using namespace castle_ext::protocols::nmea::generator;
    using namespace castle_ext::protocols::nmea::messages;

    gll_input input;
    input.latitude_deg = 47.2852333;
    input.longitude_deg = 8.5652650;
    input.hour = 9U;
    input.minute = 27U;
    input.second = 25U;
    input.status_active = true;
    input.mode = 'A';
    input.valid = true;

    char sentence[128U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(
        generate_gll(input, castle::container::string_view(TALKER_GP), sentence, sizeof(sentence), written)));

    castle::container::string_view field_storage[8U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(
        castle::container::string_view(sentence, written), field_storage, 8U, raw));

    CASTLE_SAMPLE_CHECK(gll::matches(raw));

    gll fix;
    CASTLE_SAMPLE_CHECK(gll::decode(raw, fix));
    CASTLE_SAMPLE_CHECK(fix.valid);
    CASTLE_SAMPLE_CHECK(fix.status_active);
    CASTLE_SAMPLE_CHECK((fix.latitude > 47.28) && (fix.latitude < 47.29));
    CASTLE_SAMPLE_CHECK((fix.longitude > 8.56) && (fix.longitude < 8.57));
    CASTLE_SAMPLE_CHECK(fix.mode == 'A');

    return 0;
}
