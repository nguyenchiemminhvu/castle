#include "sample_support.hpp"

#include "castle/units/scale.hpp"

#include <stdint.h>

int main()
{
    // Prefixes are exact ratios relative to the coherent SI unit.
    using per_mille = castle::math::ratio_multiply_t<castle::units::kilo, castle::units::micro>;
    static_assert(castle::math::ratio_equal<per_mille, castle::units::milli>::value, "kilo * micro == milli");

    // Time scales are the periods of castle::chrono::duration.
    using minutes_per_hour = castle::math::ratio_divide_t<castle::units::hour, castle::units::minute>;
    static_assert(minutes_per_hour::num == 60 && minutes_per_hour::den == 1, "60 min per hour");

    CASTLE_SAMPLE_CHECK(castle::units::week::num / castle::units::day::num == 7);
    CASTLE_SAMPLE_CHECK(castle::units::mega::num / castle::units::kilo::num == 1000);
    return 0;
}
