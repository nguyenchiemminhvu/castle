
#include <gtest/gtest.h>

#include <limits>

#include "castle/math/interpolation/knots.hpp"

namespace
{

using castle::size_type;
using castle::status;
namespace detail = castle::math::detail;

template <typename T>
void check_validate()
{
    const T good[] = {0, 1, 4, 9};
    const T equal_tail[] = {0, 1, 1, 9};
    const T decreasing[] = {0, 2, 1, 9};
    const T nan = std::numeric_limits<T>::quiet_NaN();
    const T nan_first[] = {nan, 1, 2};
    const T nan_middle[] = {0, nan, 2};
    const T nan_last[] = {0, 1, nan};
    const T negative[] = {-5, -1, 0, 3};

    EXPECT_EQ(detail::validate_knots(good, 4U), status::ok);
    EXPECT_EQ(detail::validate_knots(good, 2U), status::ok);
    EXPECT_EQ(detail::validate_knots(negative, 4U), status::ok);

    // Fewer than two knots or a null array.
    EXPECT_EQ(detail::validate_knots(good, 1U), status::invalid_argument);
    EXPECT_EQ(detail::validate_knots(good, 0U), status::invalid_argument);
    EXPECT_EQ(detail::validate_knots<T>(nullptr, 4U), status::invalid_argument);
    EXPECT_EQ(detail::validate_knots<T>(nullptr, 0U), status::invalid_argument);

    // Not strictly increasing.
    EXPECT_EQ(detail::validate_knots(equal_tail, 4U), status::invalid_argument);
    EXPECT_EQ(detail::validate_knots(equal_tail, 2U), status::ok);
    EXPECT_EQ(detail::validate_knots(decreasing, 4U), status::invalid_argument);
    EXPECT_EQ(detail::validate_knots(decreasing, 2U), status::ok);

    // NaN anywhere is rejected.
    EXPECT_EQ(detail::validate_knots(nan_first, 3U), status::invalid_argument);
    EXPECT_EQ(detail::validate_knots(nan_middle, 3U), status::invalid_argument);
    EXPECT_EQ(detail::validate_knots(nan_last, 3U), status::invalid_argument);
    EXPECT_EQ(detail::validate_knots(nan_last, 2U), status::ok);
}

template <typename T>
void check_find_segment()
{
    const T xs[] = {0, 1, 4, 5, 10};

    // Below the table maps to the first segment.
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(-100)), 0U);
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(-0.001)), 0U);
    // First knot and first segment interior.
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(0)), 0U);
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(0.5)), 0U);
    // Interior knots belong to the segment on their right.
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(1)), 1U);
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(3.9)), 1U);
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(4)), 2U);
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(4.5)), 2U);
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(5)), 3U);
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(9.99)), 3U);
    // Last knot and above map to the last segment.
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(10)), 3U);
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(11)), 3U);
    EXPECT_EQ(detail::find_segment(xs, 5U, static_cast<T>(1e6)), 3U);

    // Two knots: a single segment everywhere.
    EXPECT_EQ(detail::find_segment(xs, 2U, static_cast<T>(-1)), 0U);
    EXPECT_EQ(detail::find_segment(xs, 2U, static_cast<T>(0)), 0U);
    EXPECT_EQ(detail::find_segment(xs, 2U, static_cast<T>(0.5)), 0U);
    EXPECT_EQ(detail::find_segment(xs, 2U, static_cast<T>(1)), 0U);
    EXPECT_EQ(detail::find_segment(xs, 2U, static_cast<T>(7)), 0U);

    // Three knots.
    EXPECT_EQ(detail::find_segment(xs, 3U, static_cast<T>(1)), 1U);
    EXPECT_EQ(detail::find_segment(xs, 3U, static_cast<T>(4)), 1U);
    EXPECT_EQ(detail::find_segment(xs, 3U, static_cast<T>(2)), 1U);
}

TEST(MathKnots, ValidateKnotsFloat)
{
    check_validate<float>();
}

TEST(MathKnots, ValidateKnotsDouble)
{
    check_validate<double>();
}

TEST(MathKnots, FindSegmentFloat)
{
    check_find_segment<float>();
}

TEST(MathKnots, FindSegmentDouble)
{
    check_find_segment<double>();
}

TEST(MathKnots, FindSegmentAgreesWithLinearScan)
{
    const double xs[] = {-3.0, -1.5, 0.0, 0.25, 2.0, 8.0, 8.5, 20.0};
    for (int i = -80; i <= 240; ++i)
    {
        const double x = 0.1 * static_cast<double>(i);
        size_type expected = 0U;
        for (size_type k = 0U; k + 2U < 8U; ++k)
        {
            if (x >= xs[k + 1U])
            {
                expected = k + 1U;
            }
        }
        EXPECT_EQ(detail::find_segment(xs, 8U, x), expected) << "x = " << x;
    }
}

TEST(MathKnots, ExtrapolationEnum)
{
    EXPECT_NE(castle::math::extrapolation::clamp, castle::math::extrapolation::linear);
    EXPECT_EQ(static_cast<unsigned>(castle::math::extrapolation::clamp), 0U);
    EXPECT_EQ(static_cast<unsigned>(castle::math::extrapolation::linear), 1U);
    EXPECT_EQ(sizeof(castle::math::extrapolation), sizeof(unsigned char));
}

} // namespace
