#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/nmea_generator.hpp"
#include "castle_ext/protocols/nmea/messages/gsv.hpp"

int main()
{
    using namespace castle_ext::protocols::nmea;
    using namespace castle_ext::protocols::nmea::generator;
    using namespace castle_ext::protocols::nmea::messages;

    // 5 satellites: GSV packs up to 4 per sentence, so this spans 2 messages.
    gsv_satellite_record satellites[5U];
    satellites[0U].sv_id = 1U;
    satellites[0U].elevation_deg = 45;
    satellites[0U].azimuth_deg = 120;
    satellites[0U].snr = 38U;
    satellites[1U].sv_id = 3U;
    satellites[1U].elevation_deg = 30;
    satellites[1U].azimuth_deg = 210;
    satellites[1U].snr = 35U;
    satellites[2U].sv_id = 5U;
    satellites[3U].sv_id = 8U;
    satellites[4U].sv_id = 12U;
    satellites[4U].elevation_deg = 60;
    satellites[4U].azimuth_deg = 300;
    satellites[4U].snr = 42U;

    gsv_input input;
    input.satellites = castle::container::array_view<const gsv_satellite_record>(satellites, 5U);
    input.valid = true;
    CASTLE_SAMPLE_CHECK(gsv_message_count(input) == 2U);

    // First message: the leading 4 satellites.
    char sentence[128U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(generate_gsv_message(
        input, castle::container::string_view(TALKER_GP), 1U, sentence, sizeof(sentence), written)));

    castle::container::string_view field_storage[24U];
    message_view raw;
    CASTLE_SAMPLE_CHECK(decode_sentence(
        castle::container::string_view(sentence, written), field_storage, 24U, raw));
    CASTLE_SAMPLE_CHECK(gsv::matches(raw));

    gsv message_1;
    CASTLE_SAMPLE_CHECK(gsv::decode(raw, message_1));
    CASTLE_SAMPLE_CHECK(message_1.valid);
    CASTLE_SAMPLE_CHECK(message_1.num_msgs == 2U);
    CASTLE_SAMPLE_CHECK(message_1.msg_num == 1U);
    CASTLE_SAMPLE_CHECK(message_1.sats_in_view == 5U);
    CASTLE_SAMPLE_CHECK(message_1.sat_count == 4U);
    CASTLE_SAMPLE_CHECK(message_1.satellites[0U].sv_id == 1U);
    CASTLE_SAMPLE_CHECK(message_1.satellites[0U].elevation == 45);
    CASTLE_SAMPLE_CHECK(message_1.satellites[0U].azimuth == 120U);
    CASTLE_SAMPLE_CHECK(message_1.satellites[0U].snr == 38);
    CASTLE_SAMPLE_CHECK(message_1.satellites[3U].sv_id == 8U);

    // Second message: the trailing 1 satellite only.
    written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(generate_gsv_message(
        input, castle::container::string_view(TALKER_GP), 2U, sentence, sizeof(sentence), written)));
    CASTLE_SAMPLE_CHECK(decode_sentence(
        castle::container::string_view(sentence, written), field_storage, 24U, raw));
    CASTLE_SAMPLE_CHECK(gsv::matches(raw));

    gsv message_2;
    CASTLE_SAMPLE_CHECK(gsv::decode(raw, message_2));
    CASTLE_SAMPLE_CHECK(message_2.msg_num == 2U);
    CASTLE_SAMPLE_CHECK(message_2.sat_count == 1U);
    CASTLE_SAMPLE_CHECK(message_2.satellites[0U].sv_id == 12U);
    CASTLE_SAMPLE_CHECK(message_2.satellites[0U].elevation == 60);
    CASTLE_SAMPLE_CHECK(message_2.satellites[0U].snr == 42);

    return 0;
}
