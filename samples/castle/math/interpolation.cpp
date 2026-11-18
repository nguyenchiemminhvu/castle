#include "sample_support.hpp"

#include "castle/math/interpolation.hpp"
#include "castle/math/near_equal.hpp"

int main()
{
    constexpr float eps = 1.0e-5F;
    const float xs[] = {0.0F, 1.0F, 2.0F};
    const float ys[] = {0.0F, 2.0F, 8.0F};

    // Scalar kernels.
    CASTLE_SAMPLE_CHECK(castle::math::lerp(0.0F, 10.0F, 0.5F) == 5.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::catmull_rom(0.0F, 1.0F, 2.0F, 3.0F, 0.5F), 1.5F, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::bilinear(0.0F, 1.0F, 2.0F, 3.0F, 0.5F, 0.5F), 1.5F, eps));

    // Table interpolators.
    const castle::math::linear_table<float> table(xs, ys, 3U);
    CASTLE_SAMPLE_CHECK(table.valid());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(table.evaluate(1.5F), 5.0F, eps));

    castle::math::cubic_spline<float, 4U> spline;
    CASTLE_SAMPLE_CHECK(spline.build(xs, ys, 3U, castle::math::spline_kind::monotone) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.evaluate(1.0F), 2.0F, eps));
    CASTLE_SAMPLE_CHECK(spline.evaluate(3.0F, castle::math::extrapolation::clamp) == 8.0F);
    return 0;
}
