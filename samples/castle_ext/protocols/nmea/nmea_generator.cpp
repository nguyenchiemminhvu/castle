#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/nmea_generator.hpp"

int main()
{
    using namespace castle_ext::protocols::nmea;
    using namespace castle_ext::protocols::nmea::generator;

    // ---------------------------------------------------------------------
    // Generate a GGA sentence.
    // ---------------------------------------------------------------------

    gga_input gga;
    gga.hour = 9U;
    gga.minute = 27U;
    gga.second = 25U;
    gga.millisecond = 0U;

    gga.latitude_deg = 47.2852333;
    gga.longitude_deg = 8.5652650;

    gga.fix_quality = 1U;
    gga.num_satellites = 8U;
    gga.hdop = 1.0;
    gga.altitude_msl_m = 499.6;
    gga.geoid_sep_m = 48.0;
    gga.valid = true;

    char gga_sentence[128U];
    castle::size_type gga_written = 0U;

    castle::status gga_status =
        generate_gga(
            gga,
            castle::container::string_view(TALKER_GP),
            gga_sentence,
            sizeof(gga_sentence),
            gga_written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(gga_status));

    // Decode the generated sentence using the protocol-level decoder.
    castle::container::string_view gga_fields[16U];
    message_view gga_view;

    bool gga_decoded =
        decode_sentence(
            castle::container::string_view(gga_sentence, gga_written),
            gga_fields,
            16U,
            gga_view);

    CASTLE_SAMPLE_CHECK(gga_decoded);
    CASTLE_SAMPLE_CHECK(gga_view.is_talker(TALKER_GP));
    CASTLE_SAMPLE_CHECK(gga_view.is(SENTENCE_GGA));

    double latitude = 0.0;
    double longitude = 0.0;

    CASTLE_SAMPLE_CHECK(parse_latlon(gga_view.field(1U), gga_view.field(2U), latitude));
    CASTLE_SAMPLE_CHECK(parse_latlon(gga_view.field(3U), gga_view.field(4U), longitude));

    CASTLE_SAMPLE_CHECK((latitude > 47.28) && (latitude < 47.29));
    CASTLE_SAMPLE_CHECK((longitude > 8.56) && (longitude < 8.57));

    // ---------------------------------------------------------------------
    // Generate an RMC sentence.
    // ---------------------------------------------------------------------

    rmc_input rmc;
    rmc.hour = 9U;
    rmc.minute = 27U;
    rmc.second = 25U;

    rmc.day = 12U;
    rmc.month = 8U;
    rmc.year = 2026U;

    rmc.status_active = true;
    rmc.latitude_deg = 47.2852333;
    rmc.longitude_deg = 8.5652650;

    rmc.speed_knots = 3.25;
    rmc.course_true = 182.5;

    rmc.mode = 'A';
    rmc.valid = true;

    char rmc_sentence[128U];
    castle::size_type rmc_written = 0U;

    castle::status rmc_status =
        generate_rmc(
            rmc,
            castle::container::string_view(TALKER_GP),
            rmc_sentence,
            sizeof(rmc_sentence),
            rmc_written);

    CASTLE_SAMPLE_CHECK(castle::succeeded(rmc_status));

    castle::container::string_view rmc_fields[16U];
    message_view rmc_view;

    bool rmc_decoded =
        decode_sentence(
            castle::container::string_view(rmc_sentence, rmc_written),
            rmc_fields,
            16U,
            rmc_view);

    CASTLE_SAMPLE_CHECK(rmc_decoded);
    CASTLE_SAMPLE_CHECK(rmc_view.is_talker(TALKER_GP));
    CASTLE_SAMPLE_CHECK(rmc_view.is(SENTENCE_RMC));

    // ---------------------------------------------------------------------
    // Generate a GSA sentence.
    // ---------------------------------------------------------------------

    gsa_input gsa;
    gsa.op_mode = 'A';
    gsa.nav_mode = 3U;

    gsa.sat_ids[0U] = 1U;
    gsa.sat_ids[1U] = 3U;
    gsa.sat_ids[2U] = 5U;
    gsa.sat_ids[3U] = 8U;

    gsa.pdop = 1.20;
    gsa.hdop = 0.90;
    gsa.vdop = 1.50;
    gsa.valid = true;

    char gsa_sentence[128U];
    castle::size_type gsa_written = 0U;

    CASTLE_SAMPLE_CHECK(
        castle::succeeded(
            generate_gsa(
                gsa,
                castle::container::string_view(TALKER_GP),
                gsa_sentence,
                sizeof(gsa_sentence),
                gsa_written)));

    castle::container::string_view gsa_fields[20U];
    message_view gsa_view;

    CASTLE_SAMPLE_CHECK(
        decode_sentence(
            castle::container::string_view(gsa_sentence, gsa_written),
            gsa_fields,
            20U,
            gsa_view));

    CASTLE_SAMPLE_CHECK(gsa_view.is(SENTENCE_GSA));

    // ---------------------------------------------------------------------
    // Generate a VTG sentence.
    // ---------------------------------------------------------------------

    vtg_input vtg;
    vtg.course_true = 182.5;
    vtg.course_mag = 180.1;
    vtg.course_mag_valid = true;
    vtg.speed_knots = 3.25;
    vtg.speed_kmh = 6.02;
    vtg.mode = 'A';
    vtg.valid = true;

    char vtg_sentence[128U];
    castle::size_type vtg_written = 0U;

    CASTLE_SAMPLE_CHECK(
        castle::succeeded(
            generate_vtg(
                vtg,
                castle::container::string_view(TALKER_GP),
                vtg_sentence,
                sizeof(vtg_sentence),
                vtg_written)));

    // ---------------------------------------------------------------------
    // Generate a GLL sentence.
    // ---------------------------------------------------------------------

    gll_input gll;
    gll.latitude_deg = 47.2852333;
    gll.longitude_deg = 8.5652650;

    gll.hour = 9U;
    gll.minute = 27U;
    gll.second = 25U;

    gll.status_active = true;
    gll.mode = 'A';
    gll.valid = true;

    char gll_sentence[128U];
    castle::size_type gll_written = 0U;

    CASTLE_SAMPLE_CHECK(
        castle::succeeded(
            generate_gll(
                gll,
                castle::container::string_view(TALKER_GP),
                gll_sentence,
                sizeof(gll_sentence),
                gll_written)));

    // ---------------------------------------------------------------------
    // Generate a ZDA sentence.
    // ---------------------------------------------------------------------

    zda_input zda;
    zda.hour = 9U;
    zda.minute = 27U;
    zda.second = 25U;

    zda.day = 12U;
    zda.month = 8U;
    zda.year = 2026U;

    zda.tz_hour = 0;
    zda.tz_min = 0U;

    zda.valid = true;

    char zda_sentence[128U];
    castle::size_type zda_written = 0U;

    CASTLE_SAMPLE_CHECK(
        castle::succeeded(
            generate_zda(
                zda,
                castle::container::string_view(TALKER_GP),
                zda_sentence,
                sizeof(zda_sentence),
                zda_written)));

    // ---------------------------------------------------------------------
    // Generate a GSV message set.
    // ---------------------------------------------------------------------

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

    gsv_input gsv;
    gsv.satellites =
        castle::container::array_view<const gsv_satellite_record>(
            satellites,
            5U);
    gsv.valid = true;

    CASTLE_SAMPLE_CHECK(gsv_message_count(gsv) == 2U);

    for (castle::size_type message = 1U;
         message <= gsv_message_count(gsv);
         ++message)
    {
        char gsv_sentence[128U];
        castle::size_type gsv_written = 0U;

        castle::status gsv_status =
            generate_gsv_message(
                gsv,
                castle::container::string_view(TALKER_GP),
                message,
                gsv_sentence,
                sizeof(gsv_sentence),
                gsv_written);

        CASTLE_SAMPLE_CHECK(castle::succeeded(gsv_status));

        castle::container::string_view fields[32U];
        message_view view;

        CASTLE_SAMPLE_CHECK(
            decode_sentence(
                castle::container::string_view(gsv_sentence, gsv_written),
                fields,
                32U,
                view));

        CASTLE_SAMPLE_CHECK(view.is(SENTENCE_GSV));
    }

    // ---------------------------------------------------------------------
    // Invalid input is rejected.
    // ---------------------------------------------------------------------

    gga_input invalid_gga;
    invalid_gga.valid = false;

    castle::size_type rejected_written = 0U;

    CASTLE_SAMPLE_CHECK(
        generate_gga(
            invalid_gga,
            castle::container::string_view(TALKER_GP),
            gga_sentence,
            sizeof(gga_sentence),
            rejected_written)
        == castle::status::invalid_argument);

    return 0;
}
