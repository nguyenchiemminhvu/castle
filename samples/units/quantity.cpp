#include "sample_support.hpp"

#include "castle/units/quantity.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;

// A project-specific unit built from the generic engine: wheel ticks of an
// encoder with 2.5 mm per tick.
using tick_unit = cu::unit<cu::dim::length, castle::math::ratio<25, 10000>>;
using meter_unit = cu::unit<cu::dim::length>;
using speed_unit = cu::unit<cu::dimension_divide_t<cu::dim::length, cu::dim::time>>;

template <typename Rep>
using ticks = cu::quantity<tick_unit, Rep>;
template <typename Rep>
using meters = cu::quantity<meter_unit, Rep>;
template <typename Rep>
using meters_per_second = cu::quantity<speed_unit, Rep>;

meters_per_second<double> wheel_speed(ticks<int32_t> delta, castle::chrono::milliseconds window)
{
    // ticks / ms derives "2.5 mm per ms"; the implicit lossless conversion to
    // the parameter type rescales it to m/s.
    return ticks<double>(delta) / window;
}

} // namespace

int main()
{
    const meters<double> travelled = ticks<int32_t>(400);
    CASTLE_SAMPLE_CHECK(travelled.value() == 1.0);

    const meters_per_second<double> speed = wheel_speed(ticks<int32_t>(80), castle::chrono::milliseconds(20));
    CASTLE_SAMPLE_CHECK(speed.value() == 10.0);

    // Dividing compatible quantities gives a plain ratio.
    const double fraction = ticks<double>(200.0) / meters<double>(1.0);
    CASTLE_SAMPLE_CHECK(fraction == 0.5);

    // Lossy conversions must be explicit.
    const ticks<int32_t> whole = cu::quantity_cast<ticks<int32_t>>(meters<double>(0.0126));
    CASTLE_SAMPLE_CHECK(whole.value() == 5);
    CASTLE_SAMPLE_CHECK(cu::abs(meters<int32_t>(-3)) == meters<int32_t>(3));
    return 0;
}
