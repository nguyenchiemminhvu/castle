#include "sample_support.hpp"

#include "castle/math/interpolation/linear_table.hpp"
#include "castle/math/near_equal.hpp"

namespace
{

const float volts[] = {0.5F, 1.5F, 2.5F, 3.3F};
const float celsius[] = {-20.0F, 0.0F, 40.0F, 85.0F};

} // namespace

int main()
{
    constexpr float eps = 1.0e-5F;
    using castle::math::extrapolation;

    // Default-constructed table is unbound.
    castle::math::linear_table<float> empty;
    CASTLE_SAMPLE_CHECK(!empty.valid());
    CASTLE_SAMPLE_CHECK(empty.size() == 0U);

    // Constructor binds valid knots.
    const castle::math::linear_table<float> curve(volts, celsius, 4U);
    CASTLE_SAMPLE_CHECK(curve.valid());
    CASTLE_SAMPLE_CHECK(curve.size() == 4U);
    CASTLE_SAMPLE_CHECK(curve.x_min() == 0.5F);
    CASTLE_SAMPLE_CHECK(curve.x_max() == 3.3F);

    // Knots are reproduced and interior values interpolate linearly.
    CASTLE_SAMPLE_CHECK(curve.evaluate(1.5F) == 0.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(curve.evaluate(2.0F), 20.0F, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(curve(2.0F), 20.0F, eps));

    // Extrapolation: clamp holds the end values, linear extends the end segments.
    CASTLE_SAMPLE_CHECK(curve.evaluate(0.0F) == -20.0F);
    CASTLE_SAMPLE_CHECK(curve.evaluate(5.0F) == 85.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(curve.evaluate(-0.5F, extrapolation::linear), -40.0F, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(curve.evaluate(4.3F, extrapolation::linear), 85.0F + 56.25F, 1.0e-4F));

    // Derivative: right segment at knots, zero (clamp) or end slope (linear) outside.
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(curve.derivative(2.0F), 40.0F, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(curve.derivative(1.5F), 40.0F, eps));
    CASTLE_SAMPLE_CHECK(curve.derivative(0.0F) == 0.0F);
    CASTLE_SAMPLE_CHECK(curve.derivative(9.0F) == 0.0F);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(curve.derivative(0.0F, extrapolation::linear), 20.0F, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(curve.derivative(9.0F, extrapolation::linear), 56.25F, 1.0e-4F));

    // assign: success and every failure path (table is unbound afterwards).
    castle::math::linear_table<float> table;
    CASTLE_SAMPLE_CHECK(table.assign(volts, celsius, 4U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.valid());
    CASTLE_SAMPLE_CHECK(table.assign(volts, nullptr, 4U) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(!table.valid());
    CASTLE_SAMPLE_CHECK(table.assign(volts, celsius, 4U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.assign(nullptr, celsius, 4U) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(!table.valid());
    CASTLE_SAMPLE_CHECK(table.assign(volts, celsius, 1U) == castle::status::invalid_argument);
    const float flat_x[] = {0.0F, 1.0F, 1.0F};
    CASTLE_SAMPLE_CHECK(table.assign(flat_x, celsius, 3U) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(table.size() == 0U);

    // A failing constructor leaves the table unbound too.
    const castle::math::linear_table<float> broken(flat_x, celsius, 3U);
    CASTLE_SAMPLE_CHECK(!broken.valid());
    return 0;
}
