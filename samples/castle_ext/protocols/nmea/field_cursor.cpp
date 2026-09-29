#include "sample_support.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"
#include "castle_ext/protocols/nmea/nmea_generator.hpp"
#include "castle_ext/protocols/nmea/field_cursor.hpp"

int main()
{
    using namespace castle::protocols::nmea;
    using namespace castle::protocols::nmea::generator;

    // Build a known-good GGA sentence through the generator so the checksum
    // is always correct by construction (no hand-written literal to get wrong).
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
    input.dgps_age_s = 2.0;
    input.dgps_station_id = 7U;
    input.valid = true;

    char sentence[128U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(
        generate_gga(input, castle::container::string_view(TALKER_GP), sentence, sizeof(sentence), written)));

    castle::container::string_view field_storage[16U];
    message_view view;
    CASTLE_SAMPLE_CHECK(decode_sentence(
        castle::container::string_view(sentence, written), field_storage, 16U, view));
    CASTLE_SAMPLE_CHECK(view.field_count() == 14U);

    // field_cursor removes manual index bookkeeping: each next_*() consumes
    // the current field and advances, so the call order mirrors the sentence layout.
    field_cursor cursor(view);
    CASTLE_SAMPLE_CHECK(cursor.index() == 0U);

    double utc_time = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    uint32_t fix_quality = 0U;
    uint32_t num_satellites = 0U;

    // Mandatory fields are chained with &&: any failure short-circuits the rest.
    bool ok = true;
    ok = ok && cursor.next_double(utc_time);
    ok = ok && cursor.next_latlon(latitude);  // consumes lat value + N/S in one call
    ok = ok && cursor.next_latlon(longitude); // consumes lon value + E/W in one call
    ok = ok && cursor.next_uint(fix_quality);
    ok = ok && cursor.next_uint(num_satellites);
    CASTLE_SAMPLE_CHECK(ok);
    CASTLE_SAMPLE_CHECK((latitude > 47.28) && (latitude < 47.29));
    CASTLE_SAMPLE_CHECK((longitude > 8.56) && (longitude < 8.57));
    CASTLE_SAMPLE_CHECK(fix_quality == 1U);
    CASTLE_SAMPLE_CHECK(num_satellites == 8U);
    // 7 fields consumed: utc(1) + lat value/dir(2) + lon value/dir(2) + fix_quality(1) + num_sats(1).
    CASTLE_SAMPLE_CHECK(cursor.index() == 7U);

    // Optional fields use next_*_or(): they consume one field regardless of
    // whether it is empty, keeping the fallback on empty/unparsable content.
    double hdop = cursor.next_double_or(0.0);
    double altitude_msl = cursor.next_double_or(0.0);
    cursor.skip(); // altitude units, always 'M'
    double geoid_sep = cursor.next_double_or(0.0);
    cursor.skip(); // geoid separation units, always 'M'
    double dgps_age = cursor.next_double_or(-1.0);
    uint32_t dgps_station = cursor.next_uint_or(0U);

    CASTLE_SAMPLE_CHECK((hdop > 0.99) && (hdop < 1.01));
    CASTLE_SAMPLE_CHECK((altitude_msl > 499.0) && (altitude_msl < 500.0));
    CASTLE_SAMPLE_CHECK((geoid_sep > 47.0) && (geoid_sep < 49.0));
    CASTLE_SAMPLE_CHECK((dgps_age > 1.9) && (dgps_age < 2.1));
    CASTLE_SAMPLE_CHECK(dgps_station == 7U);
    CASTLE_SAMPLE_CHECK(cursor.index() == view.field_count());

    // A field that is present but not the expected type keeps the fallback,
    // instead of being treated as a hard parse error (NMEA fields are
    // routinely/legitimately empty or garbage for a given decoder).
    castle::container::string_view garbage_fields[1U] = {
        castle::container::string_view("not-a-number")
    };
    message_view garbage_view;
    garbage_view.fields = castle::container::array_view<const castle::container::string_view>(garbage_fields, 1U);
    field_cursor garbage_cursor(garbage_view);
    CASTLE_SAMPLE_CHECK(garbage_cursor.next_double_or(42.0) == 42.0);

    return 0;
}

