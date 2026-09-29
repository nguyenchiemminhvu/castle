#include "sample_support.hpp"

#include "castle/math/near_equal.hpp"
#include "castle/math/linalg/trigonometry.hpp"
#include "castle_ext/geodetic/geo_utils.hpp"

int main()
{
    const double epsilon = 1.0e-9;

    // atan2/asin wrappers fill a gap left by castle's core trigonometry header.
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(
        castle::math::radians_atan2(1.0, 1.0), 0.78539816339744830961, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(
        castle::math::radians_asin(1.0), 1.57079632679489661923, epsilon));

    // Latitude/longitude range validation.
    CASTLE_SAMPLE_CHECK(castle_ext::geodetic::is_valid_latitude(51.5));
    CASTLE_SAMPLE_CHECK(!castle_ext::geodetic::is_valid_latitude(120.0));
    CASTLE_SAMPLE_CHECK(castle_ext::geodetic::is_valid_longitude(105.8342));
    CASTLE_SAMPLE_CHECK(!castle_ext::geodetic::is_valid_longitude(200.0));

    // Clamping and wraparound normalization.
    CASTLE_SAMPLE_CHECK(castle_ext::geodetic::clamp_latitude(120.0) == 90.0);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle_ext::geodetic::normalize_longitude(200.0), -160.0, epsilon));

    return 0;
}
