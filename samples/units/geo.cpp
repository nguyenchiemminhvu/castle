#include "sample_support.hpp"

#include "castle/units/angle.hpp"
#include "castle/units/geo.hpp"

#include <stdint.h>

namespace
{

struct gps_fix
{
    castle::units::geo_degrees_e7<int32_t> lat;
    castle::units::geo_degrees_e7<int32_t> lon;
};

// The parameter types keep a latitude from being confused with a plane-angle
// heading; any geographic unit that converts losslessly is accepted.
bool accept_fix(castle::units::geo_degrees<double> lat, castle::units::geo_degrees<double> lon)
{
    return castle::units::is_valid_latitude(lat) && castle::units::is_valid_longitude(lon);
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    // u-blox UBX NAV-PVT reports int32 lat/lon in 1e-7 degree.
    const gps_fix seoul = {castle::units::geo_degrees_e7<int32_t>(375665000),
                           castle::units::geo_degrees_e7<int32_t>(1269780000)};
    const castle::units::geo_degrees<double> lat = seoul.lat;
    CASTLE_SAMPLE_CHECK(lat.value() == 37.5665);
    CASTLE_SAMPLE_CHECK(accept_fix(seoul.lat, seoul.lon));

    // Out-of-range coordinates are rejected without overflowing int32 maths.
    CASTLE_SAMPLE_CHECK(!accept_fix(castle::units::geo_degrees_e7<int32_t>(910000000), seoul.lon));
    CASTLE_SAMPLE_CHECK(!castle::units::is_valid_longitude(castle::units::geo_degrees_e7<int32_t>(2000000000)));

    // Garmin FIT semicircles: 2^30 is 90 degrees.
    const castle::units::geo_degrees<double> fit_lat = castle::units::geo_semicircles<int32_t>(1073741824);
    CASTLE_SAMPLE_CHECK(fit_lat.value() == 90.0);

    // Dead-reckoned longitude is normalised back into [-180, 180).
    const castle::units::geo_degrees<double> drifted = 190.0_geo_deg;
    CASTLE_SAMPLE_CHECK(!castle::units::is_valid_longitude(drifted));
    const castle::units::geo_degrees<double> wrapped = castle::units::wrap_signed(drifted);
    CASTLE_SAMPLE_CHECK(wrapped.value() == -170.0);
    CASTLE_SAMPLE_CHECK(castle::units::is_valid_longitude(wrapped));

    // A plane-angle heading needs an explicit cast to become a geographic angle.
    const castle::units::geo_degrees<double> as_geo =
        castle::units::quantity_cast<castle::units::geo_degrees<double>>(castle::units::degrees<double>(45.0));
    CASTLE_SAMPLE_CHECK(as_geo.value() == 45.0);

    // Geo lat/lon to arc
    const castle::units::geo_degrees<double> lon(18.0);
    CASTLE_SAMPLE_CHECK(castle::units::geo_arcseconds<double>(lon).value() == 18.0 * 3600);
    CASTLE_SAMPLE_CHECK(castle::units::geo_milliarcseconds<double>(lon).value() == 18.0 * 3600000);

    const castle::units::geo_milliarcseconds<double> lon_mas = 36'000'000.0_geo_mas;
    CASTLE_SAMPLE_CHECK(lon_mas.value() == 36'000'000.0);
    CASTLE_SAMPLE_CHECK(castle::units::geo_degrees<double>(lon_mas).value() == 10.0);

    return 0;
}
