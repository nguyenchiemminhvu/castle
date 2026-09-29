#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/nmea_generator.hpp"
#include "castle_ext/protocols/nmea/messages/gsa.hpp"

int main()
{
    using namespace castle::protocols::nmea;
    using namespace castle::protocols::nmea::generator;
    using namespace castle::protocols::nmea::messages;

    gsa_input input;
    input.op_mode = 'A';
    input.nav_mode = 3U;
    input.sat_ids[0U] = 1U;
    input.sat_ids[1U] = 3U;
    input.sat_ids[2U] = 5U;
    input.sat_ids[3U] = 8U;
    input.pdop = 1.20;
    input.hdop = 0.90;
    input.vdop = 1.50;
    input.valid = true;

    char sentence[128U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(
        generate_gsa(input, castle::container::string_view(TALKER_GP), sentence, sizeof(sentence), written)));

    castle::container::string_view field_storage[20U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(
        castle::container::string_view(sentence, written), field_storage, 20U, raw));

    CASTLE_SAMPLE_CHECK(gsa::matches(raw));

    gsa fix;
    CASTLE_SAMPLE_CHECK(gsa::decode(raw, fix));
    CASTLE_SAMPLE_CHECK(fix.valid);
    CASTLE_SAMPLE_CHECK(fix.op_mode == gsa_op_mode::auto_mode);
    CASTLE_SAMPLE_CHECK(fix.nav_mode == gsa_nav_mode::fix_3d);
    CASTLE_SAMPLE_CHECK(fix.sat_ids[0U] == 1U);
    CASTLE_SAMPLE_CHECK(fix.sat_ids[1U] == 3U);
    CASTLE_SAMPLE_CHECK(fix.sat_ids[2U] == 5U);
    CASTLE_SAMPLE_CHECK(fix.sat_ids[3U] == 8U);
    // Unused satellite slots stay zero-initialized.
    CASTLE_SAMPLE_CHECK(fix.sat_ids[4U] == 0U);
    CASTLE_SAMPLE_CHECK((fix.pdop > 1.19) && (fix.pdop < 1.21));
    CASTLE_SAMPLE_CHECK((fix.hdop > 0.89) && (fix.hdop < 0.91));
    CASTLE_SAMPLE_CHECK((fix.vdop > 1.49) && (fix.vdop < 1.51));

    return 0;
}
