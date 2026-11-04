#include "sample_support.hpp"

#include "castle/math/interpolation/knots.hpp"

int main()
{
    const float xs[] = {0.0F, 1.0F, 4.0F};
    const float bad[] = {0.0F, 2.0F, 2.0F};

    // validate_knots: ok and every failure class.
    CASTLE_SAMPLE_CHECK(castle::math::detail::validate_knots(xs, 3U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::detail::validate_knots(bad, 3U) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(castle::math::detail::validate_knots(xs, 1U) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(castle::math::detail::validate_knots<float>(nullptr, 3U) == castle::status::invalid_argument);

    // find_segment: below, at first knot, interior, at last knot, above.
    CASTLE_SAMPLE_CHECK(castle::math::detail::find_segment(xs, 3U, -1.0F) == 0U);
    CASTLE_SAMPLE_CHECK(castle::math::detail::find_segment(xs, 3U, 0.0F) == 0U);
    CASTLE_SAMPLE_CHECK(castle::math::detail::find_segment(xs, 3U, 0.5F) == 0U);
    CASTLE_SAMPLE_CHECK(castle::math::detail::find_segment(xs, 3U, 1.0F) == 1U);
    CASTLE_SAMPLE_CHECK(castle::math::detail::find_segment(xs, 3U, 2.0F) == 1U);
    CASTLE_SAMPLE_CHECK(castle::math::detail::find_segment(xs, 3U, 4.0F) == 1U);
    CASTLE_SAMPLE_CHECK(castle::math::detail::find_segment(xs, 3U, 9.0F) == 1U);

    // extrapolation policy values.
    const castle::math::extrapolation clamp = castle::math::extrapolation::clamp;
    const castle::math::extrapolation linear = castle::math::extrapolation::linear;
    CASTLE_SAMPLE_CHECK(clamp != linear);
    return 0;
}
