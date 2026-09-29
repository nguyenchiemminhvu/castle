#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/nmea_generator.hpp"
#include "castle/utility/string_builder.hpp"

namespace
{

using namespace castle_ext::protocols::nmea;
using namespace castle_ext::protocols::nmea::generator;
using castle::container::string_view;

CASTLE_NODISCARD bool decode(
    string_view line, message_view& view, string_view* storage, castle::size_type capacity)
{
    return decode_sentence(line, storage, capacity, view);
}

CASTLE_NODISCARD double parse_field_double(CASTLE_CONST message_view& view, castle::size_type index)
{
    double value = 0.0;
    EXPECT_TRUE(parse_double(view.field(index), value));
    return value;
}

// ---------------------------------------------------------------------------
// Field-formatting helper functions (direct, template-instantiated with string_builder)
// ---------------------------------------------------------------------------

TEST(NmeaGeneratorHelpers, AppendPaddedVariousWidths)
{
    castle::string_builder<32U> builder;
    append_padded(builder, 5U, 2U);   // value < divisor -> one leading zero
    append_padded(builder, 12U, 2U);  // value >= divisor -> no leading zero
    append_padded(builder, 5U, 1U);   // width 1 -> loop body never runs
    append_padded(builder, 5U, 3U);   // two leading zeros
    EXPECT_EQ(builder.view(), string_view("05125005"));
}

TEST(NmeaGeneratorHelpers, AppendUtcTimeOneDecimalDigit)
{
    castle::string_builder<32U> builder;
    append_utc_time(builder, 9U, 7U, 5U, 500U, 1U);
    EXPECT_EQ(builder.view(), string_view("090705.5"));
}

TEST(NmeaGeneratorHelpers, AppendUtcTimeDefaultTwoDecimalDigits)
{
    castle::string_builder<32U> builder;
    append_utc_time(builder, 9U, 27U, 25U, 500U);
    EXPECT_EQ(builder.view(), string_view("092725.50"));
}

TEST(NmeaGeneratorHelpers, IndicatorFunctions)
{
    EXPECT_EQ(ns_indicator(1.0), 'N');
    EXPECT_EQ(ns_indicator(-1.0), 'S');
    EXPECT_EQ(ns_indicator(0.0), 'N');
    EXPECT_EQ(ew_indicator(1.0), 'E');
    EXPECT_EQ(ew_indicator(-1.0), 'W');
    EXPECT_EQ(ew_indicator(0.0), 'E');
}

// ---------------------------------------------------------------------------
// generate_gga
// ---------------------------------------------------------------------------

TEST(NmeaGeneratorGga, InvalidInputIsInvalidArgument)
{
    gga_input input;
    input.valid = false;
    char output[128U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gga(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGga, NullOutputIsInvalidArgument)
{
    gga_input input;
    input.valid = true;
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gga(input, string_view(TALKER_GP), nullptr, 128U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGga, OutputCapacityTooSmallIsFull)
{
    gga_input input;
    input.valid = true;
    input.latitude_deg = 47.2852333;
    input.longitude_deg = 8.5652650;
    char output[4U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gga(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGga, SentenceLengthTruncationIsFull)
{
    gga_input input;
    input.valid = true;
    input.hdop = 1.2;
    input.dgps_age_s = 5.0;
    input.dgps_station_id = 1234U;
    char output[256U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ((generate_gga<16U>(input, string_view(TALKER_GP), output, sizeof(output), written)), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGga, FullFeaturedFieldsRoundTrip)
{
    gga_input input;
    input.hour = 9U; input.minute = 27U; input.second = 25U; input.millisecond = 500U;
    input.latitude_deg = -47.2852333;  // south, minutes >= 10 (no synthetic leading zero)
    input.longitude_deg = 8.5652650;   // east, minutes >= 10
    input.fix_quality = 1U;
    input.num_satellites = 8U;
    input.hdop = 1.2;
    input.altitude_msl_m = 499.6;
    input.geoid_sep_m = 48.0;
    input.dgps_age_s = 5.0;
    input.dgps_station_id = 1234U;
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_gga(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);
    ASSERT_GT(written, 0U);

    message_view view;
    string_view storage[20U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 20U));
    ASSERT_TRUE(view.is_talker(string_view("GP")));
    ASSERT_TRUE(view.is(string_view("GGA")));
    ASSERT_EQ(view.field_count(), 14U);

    EXPECT_EQ(view.field(0U), string_view("092725.50"));
    double lat = 0.0;
    ASSERT_TRUE(parse_latlon(view.field(1U), view.field(2U), lat));
    EXPECT_NEAR(lat, -47.2852333, 1e-4);
    double lon = 0.0;
    ASSERT_TRUE(parse_latlon(view.field(3U), view.field(4U), lon));
    EXPECT_NEAR(lon, 8.5652650, 1e-4);
    EXPECT_EQ(view.field(5U), string_view("1"));
    EXPECT_EQ(view.field(6U), string_view("08"));
    EXPECT_FALSE(view.field(7U).empty());
    EXPECT_NEAR(parse_field_double(view, 7U), 1.2, 1e-6);
    EXPECT_NEAR(parse_field_double(view, 8U), 499.6, 1e-6);
    EXPECT_EQ(view.field(9U), string_view("M"));
    EXPECT_NEAR(parse_field_double(view, 10U), 48.0, 1e-6);
    EXPECT_EQ(view.field(11U), string_view("M"));
    EXPECT_FALSE(view.field(12U).empty());
    EXPECT_NEAR(parse_field_double(view, 12U), 5.0, 1e-6);
    EXPECT_EQ(view.field(13U), string_view("1234"));
}

TEST(NmeaGeneratorGga, MinimalOptionalFieldsAreEmpty)
{
    gga_input input;
    input.latitude_deg = 1.05;   // north, minutes < 10 -> synthetic leading zero
    input.longitude_deg = -1.05; // west, minutes < 10
    input.hdop = 0.0;            // omitted
    input.dgps_age_s = -1.0;     // omitted, station id also omitted
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_gga(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[20U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 20U));
    ASSERT_EQ(view.field_count(), 14U);

    double lat = 0.0;
    ASSERT_TRUE(parse_latlon(view.field(1U), view.field(2U), lat));
    EXPECT_NEAR(lat, 1.05, 1e-4);
    EXPECT_EQ(view.field(2U), string_view("N"));
    double lon = 0.0;
    ASSERT_TRUE(parse_latlon(view.field(3U), view.field(4U), lon));
    EXPECT_NEAR(lon, -1.05, 1e-4);
    EXPECT_EQ(view.field(4U), string_view("W"));
    EXPECT_TRUE(view.field(7U).empty());
    EXPECT_TRUE(view.field(12U).empty());
    EXPECT_TRUE(view.field(13U).empty());
}

// ---------------------------------------------------------------------------
// generate_rmc
// ---------------------------------------------------------------------------

TEST(NmeaGeneratorRmc, InvalidInputIsInvalidArgument)
{
    rmc_input input;
    input.valid = false;
    char output[128U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_rmc(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorRmc, NullOutputIsInvalidArgument)
{
    rmc_input input;
    input.valid = true;
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_rmc(input, string_view(TALKER_GP), nullptr, 128U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorRmc, OutputCapacityTooSmallIsFull)
{
    rmc_input input;
    input.valid = true;
    char output[4U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_rmc(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorRmc, ActiveWithMagneticVariationRoundTrip)
{
    rmc_input input;
    input.hour = 9U; input.minute = 27U; input.second = 25U; input.millisecond = 0U;
    input.status_active = true;
    input.latitude_deg = 1.05;    // north, minutes < 10
    input.longitude_deg = -8.56;  // west
    input.speed_knots = 12.3;
    input.course_true = 45.6;
    input.day = 15U; input.month = 6U; input.year = 2024U;
    input.mag_variation = -3.2;
    input.mag_var_valid = true;
    input.mode = 'A';
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_rmc(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[16U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 16U));
    ASSERT_TRUE(view.is(string_view("RMC")));
    ASSERT_EQ(view.field_count(), 12U);

    EXPECT_EQ(view.field(0U), string_view("092725.00"));
    EXPECT_EQ(view.field(1U), string_view("A"));
    double lat = 0.0;
    ASSERT_TRUE(parse_latlon(view.field(2U), view.field(3U), lat));
    EXPECT_NEAR(lat, 1.05, 1e-4);
    double lon = 0.0;
    ASSERT_TRUE(parse_latlon(view.field(4U), view.field(5U), lon));
    EXPECT_NEAR(lon, -8.56, 1e-4);
    EXPECT_EQ(view.field(5U), string_view("W"));
    EXPECT_NEAR(parse_field_double(view, 6U), 12.3, 1e-6);
    EXPECT_NEAR(parse_field_double(view, 7U), 45.6, 1e-6);
    EXPECT_EQ(view.field(8U), string_view("150624"));
    EXPECT_NEAR(parse_field_double(view, 9U), 3.2, 1e-6);
    EXPECT_EQ(view.field(10U), string_view("W"));
    EXPECT_EQ(view.field(11U), string_view("A"));
}

TEST(NmeaGeneratorRmc, VoidWithoutMagneticVariationRoundTrip)
{
    rmc_input input;
    input.status_active = false;
    input.latitude_deg = -1.05;
    input.longitude_deg = 8.56;
    input.mag_var_valid = false;
    input.mode = 'N';
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_rmc(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[16U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 16U));
    ASSERT_EQ(view.field_count(), 12U);

    EXPECT_EQ(view.field(1U), string_view("V"));
    EXPECT_TRUE(view.field(9U).empty());
    EXPECT_TRUE(view.field(10U).empty());
    EXPECT_EQ(view.field(11U), string_view("N"));
}

// ---------------------------------------------------------------------------
// generate_gsa
// ---------------------------------------------------------------------------

TEST(NmeaGeneratorGsa, InvalidInputIsInvalidArgument)
{
    gsa_input input;
    input.valid = false;
    char output[128U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gsa(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGsa, NullOutputIsInvalidArgument)
{
    gsa_input input;
    input.valid = true;
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gsa(input, string_view(TALKER_GP), nullptr, 128U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGsa, OutputCapacityTooSmallIsFull)
{
    gsa_input input;
    input.valid = true;
    char output[4U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gsa(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGsa, FullFeaturedWithSystemIdRoundTrip)
{
    gsa_input input;
    input.op_mode = 'A';
    input.nav_mode = 3U;
    input.sat_ids[0U] = 1U; input.sat_ids[1U] = 2U; input.sat_ids[2U] = 3U;
    input.pdop = 1.5; input.hdop = 0.9; input.vdop = 1.1;
    input.system_id = 1U;
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_gsa(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[20U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 20U));
    ASSERT_TRUE(view.is(string_view("GSA")));
    ASSERT_EQ(view.field_count(), 18U);

    EXPECT_EQ(view.field(0U), string_view("A"));
    EXPECT_EQ(view.field(1U), string_view("3"));
    EXPECT_EQ(view.field(2U), string_view("01"));
    EXPECT_EQ(view.field(3U), string_view("02"));
    EXPECT_EQ(view.field(4U), string_view("03"));
    EXPECT_TRUE(view.field(5U).empty());
    EXPECT_TRUE(view.field(13U).empty());
    EXPECT_NEAR(parse_field_double(view, 14U), 1.5, 1e-6);
    EXPECT_NEAR(parse_field_double(view, 15U), 0.9, 1e-6);
    EXPECT_NEAR(parse_field_double(view, 16U), 1.1, 1e-6);
    EXPECT_EQ(view.field(17U), string_view("1"));
}

TEST(NmeaGeneratorGsa, MinimalWithoutSystemIdOmitsField)
{
    gsa_input input;
    input.op_mode = 'M';
    input.nav_mode = 1U;
    input.pdop = 0.0; input.hdop = 0.0; input.vdop = 0.0;
    input.system_id = 0U;
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_gsa(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[20U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 20U));
    ASSERT_EQ(view.field_count(), 17U);

    for (castle::size_type index = 2U; index < 14U; ++index)
    {
        EXPECT_TRUE(view.field(index).empty());
    }
    EXPECT_TRUE(view.field(14U).empty());
    EXPECT_TRUE(view.field(15U).empty());
    EXPECT_TRUE(view.field(16U).empty());
}

// ---------------------------------------------------------------------------
// generate_vtg
// ---------------------------------------------------------------------------

TEST(NmeaGeneratorVtg, InvalidInputIsInvalidArgument)
{
    vtg_input input;
    input.valid = false;
    char output[128U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_vtg(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorVtg, NullOutputIsInvalidArgument)
{
    vtg_input input;
    input.valid = true;
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_vtg(input, string_view(TALKER_GP), nullptr, 128U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorVtg, OutputCapacityTooSmallIsFull)
{
    vtg_input input;
    input.valid = true;
    char output[4U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_vtg(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorVtg, WithMagneticCourseRoundTrip)
{
    vtg_input input;
    input.course_true = 45.6;
    input.course_mag = 40.1;
    input.course_mag_valid = true;
    input.speed_knots = 12.3;
    input.speed_kmh = 22.8;
    input.mode = 'A';
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_vtg(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[12U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 12U));
    ASSERT_TRUE(view.is(string_view("VTG")));
    ASSERT_EQ(view.field_count(), 9U);

    EXPECT_NEAR(parse_field_double(view, 0U), 45.6, 1e-6);
    EXPECT_EQ(view.field(1U), string_view("T"));
    EXPECT_NEAR(parse_field_double(view, 2U), 40.1, 1e-6);
    EXPECT_EQ(view.field(3U), string_view("M"));
    EXPECT_NEAR(parse_field_double(view, 4U), 12.3, 1e-6);
    EXPECT_EQ(view.field(5U), string_view("N"));
    EXPECT_NEAR(parse_field_double(view, 6U), 22.8, 1e-6);
    EXPECT_EQ(view.field(7U), string_view("K"));
    EXPECT_EQ(view.field(8U), string_view("A"));
}

TEST(NmeaGeneratorVtg, WithoutMagneticCourseFieldIsEmpty)
{
    vtg_input input;
    input.course_mag_valid = false;
    input.mode = 'N';
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_vtg(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[12U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 12U));
    ASSERT_EQ(view.field_count(), 9U);
    EXPECT_TRUE(view.field(2U).empty());
}

// ---------------------------------------------------------------------------
// generate_gll
// ---------------------------------------------------------------------------

TEST(NmeaGeneratorGll, InvalidInputIsInvalidArgument)
{
    gll_input input;
    input.valid = false;
    char output[128U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gll(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGll, NullOutputIsInvalidArgument)
{
    gll_input input;
    input.valid = true;
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gll(input, string_view(TALKER_GP), nullptr, 128U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGll, OutputCapacityTooSmallIsFull)
{
    gll_input input;
    input.valid = true;
    char output[4U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gll(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGll, ActiveStatusRoundTrip)
{
    gll_input input;
    input.latitude_deg = 47.2852333;
    input.longitude_deg = -8.5652650;
    input.hour = 9U; input.minute = 27U; input.second = 25U; input.millisecond = 0U;
    input.status_active = true;
    input.mode = 'A';
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_gll(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[8U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 8U));
    ASSERT_TRUE(view.is(string_view("GLL")));
    ASSERT_EQ(view.field_count(), 7U);

    double lat = 0.0;
    ASSERT_TRUE(parse_latlon(view.field(0U), view.field(1U), lat));
    EXPECT_NEAR(lat, 47.2852333, 1e-4);
    double lon = 0.0;
    ASSERT_TRUE(parse_latlon(view.field(2U), view.field(3U), lon));
    EXPECT_NEAR(lon, -8.5652650, 1e-4);
    EXPECT_EQ(view.field(4U), string_view("092725.00"));
    EXPECT_EQ(view.field(5U), string_view("A"));
    EXPECT_EQ(view.field(6U), string_view("A"));
}

TEST(NmeaGeneratorGll, VoidStatusRoundTrip)
{
    gll_input input;
    input.status_active = false;
    input.mode = 'V';
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_gll(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[8U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 8U));
    EXPECT_EQ(view.field(5U), string_view("V"));
}

// ---------------------------------------------------------------------------
// generate_zda
// ---------------------------------------------------------------------------

TEST(NmeaGeneratorZda, InvalidInputIsInvalidArgument)
{
    zda_input input;
    input.valid = false;
    char output[128U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_zda(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorZda, NullOutputIsInvalidArgument)
{
    zda_input input;
    input.valid = true;
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_zda(input, string_view(TALKER_GP), nullptr, 128U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorZda, OutputCapacityTooSmallIsFull)
{
    zda_input input;
    input.valid = true;
    char output[4U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_zda(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorZda, NegativeTimezoneRoundTrip)
{
    zda_input input;
    input.hour = 9U; input.minute = 27U; input.second = 25U; input.millisecond = 0U;
    input.day = 15U; input.month = 6U; input.year = 2024U;
    input.tz_hour = -5;
    input.tz_min = 30U;
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_zda(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[8U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 8U));
    ASSERT_TRUE(view.is(string_view("ZDA")));
    ASSERT_EQ(view.field_count(), 6U);

    EXPECT_EQ(view.field(0U), string_view("092725.00"));
    EXPECT_EQ(view.field(1U), string_view("15"));
    EXPECT_EQ(view.field(2U), string_view("06"));
    EXPECT_EQ(view.field(3U), string_view("2024"));
    EXPECT_EQ(view.field(4U), string_view("-05"));
    EXPECT_EQ(view.field(5U), string_view("30"));
}

TEST(NmeaGeneratorZda, NonNegativeTimezoneRoundTrip)
{
    zda_input input;
    input.day = 1U; input.month = 1U; input.year = 2024U;
    input.tz_hour = 0;
    input.tz_min = 0U;
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_zda(input, string_view(TALKER_GP), output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[8U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 8U));
    EXPECT_EQ(view.field(4U), string_view("00"));
}

// ---------------------------------------------------------------------------
// GSV: gsv_message_count
// ---------------------------------------------------------------------------

TEST(NmeaGeneratorGsv, MessageCount)
{
    gsv_input input;

    input.satellites = castle::container::array_view<CASTLE_CONST gsv_satellite_record>(nullptr, 0U);
    EXPECT_EQ(gsv_message_count(input), 1U);

    gsv_satellite_record four[4U] = {};
    input.satellites = castle::container::array_view<CASTLE_CONST gsv_satellite_record>(four, 4U);
    EXPECT_EQ(gsv_message_count(input), 1U);

    gsv_satellite_record five[5U] = {};
    input.satellites = castle::container::array_view<CASTLE_CONST gsv_satellite_record>(five, 5U);
    EXPECT_EQ(gsv_message_count(input), 2U);
}

// ---------------------------------------------------------------------------
// generate_gsv_message
// ---------------------------------------------------------------------------

TEST(NmeaGeneratorGsv, InvalidInputIsInvalidArgument)
{
    gsv_input input;
    input.valid = false;
    char output[128U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gsv_message(input, string_view(TALKER_GP), 1U, output, sizeof(output), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGsv, NullOutputIsInvalidArgument)
{
    gsv_input input;
    input.valid = true;
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gsv_message(input, string_view(TALKER_GP), 1U, nullptr, 128U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGsv, ZeroMessageNumberIsInvalidArgument)
{
    gsv_input input;
    input.valid = true;
    char output[128U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gsv_message(input, string_view(TALKER_GP), 0U, output, sizeof(output), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGsv, OutOfRangeMessageNumberIsInvalidArgument)
{
    gsv_input input;
    input.valid = true;
    char output[128U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gsv_message(input, string_view(TALKER_GP), 2U, output, sizeof(output), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGsv, OutputCapacityTooSmallIsFull)
{
    gsv_input input;
    input.valid = true;
    char output[2U];
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(generate_gsv_message(input, string_view(TALKER_GP), 1U, output, sizeof(output), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(NmeaGeneratorGsv, EmptySatelliteListProducesHeaderOnlyMessage)
{
    gsv_input input;
    input.satellites = castle::container::array_view<CASTLE_CONST gsv_satellite_record>(nullptr, 0U);
    input.valid = true;

    char output[128U];
    castle::size_type written = 0U;
    ASSERT_EQ(generate_gsv_message(input, string_view(TALKER_GP), 1U, output, sizeof(output), written), castle::status::ok);

    message_view view;
    string_view storage[20U];
    ASSERT_TRUE(decode(string_view(output, written), view, storage, 20U));
    ASSERT_TRUE(view.is(string_view("GSV")));
    ASSERT_EQ(view.field_count(), 3U);
    EXPECT_EQ(view.field(0U), string_view("1"));
    EXPECT_EQ(view.field(1U), string_view("1"));
    EXPECT_EQ(view.field(2U), string_view("00"));
}

TEST(NmeaGeneratorGsv, FiveSatellitesSplitAcrossTwoMessages)
{
    gsv_satellite_record satellites[5U] = {
        {1U, -1, 10, 0U},    // elevation omitted, snr omitted
        {2U, 45, 90, 38U},   // elevation present, snr present
        {3U, 0, 180, 1U},
        {4U, 89, 270, 99U},
        {5U, 30, 359, 0U},   // in message 2, snr omitted
    };
    gsv_input input;
    input.satellites = castle::container::array_view<CASTLE_CONST gsv_satellite_record>(satellites, 5U);
    input.valid = true;

    ASSERT_EQ(gsv_message_count(input), 2U);

    char output[256U];
    castle::size_type written = 0U;

    ASSERT_EQ(generate_gsv_message(input, string_view(TALKER_GP), 1U, output, sizeof(output), written), castle::status::ok);
    {
        message_view view;
        string_view storage[20U];
        ASSERT_TRUE(decode(string_view(output, written), view, storage, 20U));
        ASSERT_EQ(view.field_count(), 3U + 4U * 4U);
        EXPECT_EQ(view.field(0U), string_view("2"));
        EXPECT_EQ(view.field(1U), string_view("1"));
        EXPECT_EQ(view.field(2U), string_view("05"));

        EXPECT_EQ(view.field(3U), string_view("1"));
        EXPECT_TRUE(view.field(4U).empty());
        EXPECT_EQ(view.field(5U), string_view("10"));
        EXPECT_TRUE(view.field(6U).empty());

        EXPECT_EQ(view.field(7U), string_view("2"));
        EXPECT_EQ(view.field(8U), string_view("45"));
        EXPECT_EQ(view.field(9U), string_view("90"));
        EXPECT_EQ(view.field(10U), string_view("38"));
    }

    ASSERT_EQ(generate_gsv_message(input, string_view(TALKER_GP), 2U, output, sizeof(output), written), castle::status::ok);
    {
        message_view view;
        string_view storage[20U];
        ASSERT_TRUE(decode(string_view(output, written), view, storage, 20U));
        ASSERT_EQ(view.field_count(), 3U + 4U);
        EXPECT_EQ(view.field(0U), string_view("2"));
        EXPECT_EQ(view.field(1U), string_view("2"));
        EXPECT_EQ(view.field(2U), string_view("05"));
        EXPECT_EQ(view.field(3U), string_view("5"));
        EXPECT_EQ(view.field(4U), string_view("30"));
        EXPECT_EQ(view.field(5U), string_view("359"));
        EXPECT_TRUE(view.field(6U).empty());
    }
}

TEST(NmeaGeneratorOthers, GeneratesAndDecodesGllSentence)
{
    gll_input input;
    input.latitude_deg = 47.2852333;
    input.longitude_deg = -8.5652650;
    input.hour = 9U;
    input.minute = 27U;
    input.second = 25U;
    input.valid = true;
    input.status_active = true;
    input.mode = 'A';

    char output[128U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(generate_gll(input, string_view("GP"), output, sizeof(output), written), castle::status::ok);
    EXPECT_GT(written, 0U);

    message_view view;
    string_view storage[8U];
    ASSERT_TRUE(decode_sentence(string_view(output, written), storage, 8U, view));
    EXPECT_TRUE(view.is(string_view("GLL")));
    EXPECT_EQ(view.field(5U), string_view("A"));
}

} // namespace
