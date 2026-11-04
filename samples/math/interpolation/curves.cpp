#include "sample_support.hpp"

#include "castle/math/interpolation/curves.hpp"
#include "castle/math/near_equal.hpp"

int main()
{
    constexpr float eps = 1.0e-5F;

    // bilinear: corners, centre and extrapolation along x.
    CASTLE_SAMPLE_CHECK(castle::math::bilinear(1.0F, 2.0F, 3.0F, 4.0F, 0.0F, 0.0F) == 1.0F);
    CASTLE_SAMPLE_CHECK(castle::math::bilinear(1.0F, 2.0F, 3.0F, 4.0F, 1.0F, 0.0F) == 2.0F);
    CASTLE_SAMPLE_CHECK(castle::math::bilinear(1.0F, 2.0F, 3.0F, 4.0F, 0.0F, 1.0F) == 3.0F);
    CASTLE_SAMPLE_CHECK(castle::math::bilinear(1.0F, 2.0F, 3.0F, 4.0F, 1.0F, 1.0F) == 4.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::bilinear(1.0F, 2.0F, 3.0F, 4.0F, 0.5F, 0.5F), 2.5F, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::bilinear(0.0F, 1.0F, 0.0F, 1.0F, 2.0F, 0.5F), 2.0F, eps));

    // cubic_hermite: end values, end tangents and a straight line.
    CASTLE_SAMPLE_CHECK(castle::math::cubic_hermite(1.0F, 5.0F, 4.0F, -3.0F, 0.0F) == 1.0F);
    CASTLE_SAMPLE_CHECK(castle::math::cubic_hermite(1.0F, 5.0F, 4.0F, -3.0F, 1.0F) == 4.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::cubic_hermite(0.0F, 3.0F, 3.0F, 3.0F, 0.5F), 1.5F, eps));

    // cubic_hermite_derivative: end tangents and finite-difference agreement.
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::cubic_hermite_derivative(1.0F, 5.0F, 4.0F, -3.0F, 0.0F), 5.0F, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::cubic_hermite_derivative(1.0F, 5.0F, 4.0F, -3.0F, 1.0F), -3.0F, eps));
    {
        constexpr double h = 1.0e-6;
        const double fd = (castle::math::cubic_hermite(1.0, 5.0, 4.0, -3.0, 0.3 + h)
                           - castle::math::cubic_hermite(1.0, 5.0, 4.0, -3.0, 0.3 - h)) / (2.0 * h);
        CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::cubic_hermite_derivative(1.0, 5.0, 4.0, -3.0, 0.3), fd, 1.0e-6));
    }

    // catmull_rom: segment end points and the midpoint of a straight line.
    CASTLE_SAMPLE_CHECK(castle::math::catmull_rom(0.0F, 1.0F, 2.0F, 3.0F, 0.0F) == 1.0F);
    CASTLE_SAMPLE_CHECK(castle::math::catmull_rom(0.0F, 1.0F, 2.0F, 3.0F, 1.0F) == 2.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::catmull_rom(0.0F, 1.0F, 2.0F, 3.0F, 0.5F), 1.5F, eps));

    // cubic_bezier: end control values and a symmetric S curve.
    CASTLE_SAMPLE_CHECK(castle::math::cubic_bezier(0.0F, 0.0F, 1.0F, 1.0F, 0.0F) == 0.0F);
    CASTLE_SAMPLE_CHECK(castle::math::cubic_bezier(0.0F, 0.0F, 1.0F, 1.0F, 1.0F) == 1.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::cubic_bezier(0.0F, 0.0F, 1.0F, 1.0F, 0.5F), 0.5F, eps));

    // Everything is constexpr.
    constexpr float mid = castle::math::catmull_rom(0.0F, 1.0F, 2.0F, 3.0F, 0.5F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(mid, 1.5F, eps));
    return 0;
}
