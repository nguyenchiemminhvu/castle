#include "sample_support.hpp"

#include "castle/math/near_equal.hpp"
#include "castle/utility/string_builder.hpp"
#include "castle_ext/geodetic/coord.hpp"

int main()
{
    using castle::geodetic::geo_point;

    // A validated latitude/longitude pair, usable at compile time.
    constexpr geo_point<double> hanoi(21.0278, 105.8342);
    CASTLE_SAMPLE_CHECK(hanoi.latitude() == 21.0278);
    CASTLE_SAMPLE_CHECK(hanoi.longitude() == 105.8342);

    // Decimal degrees <-> Degrees-Minutes-Seconds, entirely constexpr.
    constexpr castle::geodetic::dms<double> lat_dms = castle::geodetic::degrees_to_dms(hanoi.latitude());
    CASTLE_SAMPLE_CHECK(!lat_dms.negative);
    CASTLE_SAMPLE_CHECK(lat_dms.degrees == 21U);
    CASTLE_SAMPLE_CHECK(lat_dms.minutes == 1U);

    constexpr double lat_round_trip = castle::geodetic::dms_to_degrees(lat_dms);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(lat_round_trip, hanoi.latitude(), 1.0e-9));

    // Decimal degrees <-> Degrees-Decimal-Minutes.
    constexpr castle::geodetic::ddm<double> lon_ddm = castle::geodetic::degrees_to_ddm(hanoi.longitude());
    constexpr double lon_round_trip = castle::geodetic::ddm_to_degrees(lon_ddm);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(lon_round_trip, hanoi.longitude(), 1.0e-9));

    // Angle unit conversions (arcseconds, milliarcseconds, gradians, turns).
    CASTLE_SAMPLE_CHECK(castle::geodetic::degrees_to_arcseconds(1.0) == 3600.0);
    CASTLE_SAMPLE_CHECK(castle::geodetic::degrees_to_milliarcseconds(1.0) == 3600000.0);
    CASTLE_SAMPLE_CHECK(castle::geodetic::degrees_to_gradians(90.0) == 100.0);
    CASTLE_SAMPLE_CHECK(castle::geodetic::degrees_to_turns(360.0) == 1.0);

    // NATO/GEOREF-style DMS text formatting via castle::basic_string_builder.
    castle::string_builder<32U> builder;
    castle::geodetic::append_dms(builder, lat_dms, 'N', 'S');
    CASTLE_SAMPLE_CHECK(builder.size() > 0U);
    CASTLE_SAMPLE_CHECK(builder.c_str()[builder.size() - 1U] == 'N');

    return 0;
}
