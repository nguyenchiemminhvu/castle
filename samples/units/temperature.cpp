#include "sample_support.hpp"

#include "castle/units/temperature.hpp"

#include <stdint.h>

namespace
{

// A thermostat reads an int16 sensor in 0.1 degC steps; the typed parameters
// compare in the common unit without manual scaling.
bool heater_on(castle::units::deci_celsius<int16_t> reading, castle::units::celsius<int32_t> setpoint)
{
    return reading < setpoint;
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    const castle::units::deci_celsius<int16_t> raw(253);
    CASTLE_SAMPLE_CHECK(!heater_on(raw, 25_degC));
    CASTLE_SAMPLE_CHECK(heater_on(raw, castle::units::celsius<int32_t>(26)));

    // Floating conversions apply the offset implicitly.
    const castle::units::celsius<double> room = raw;
    CASTLE_SAMPLE_CHECK(room.value() > 25.299 && room.value() < 25.301);
    const castle::units::kelvin<double> room_k = raw;
    CASTLE_SAMPLE_CHECK(room_k.value() > 298.449 && room_k.value() < 298.451);

    // Integral conversions truncate and need an explicit step.
    const castle::units::kelvin<int32_t> coarse(castle::units::celsius<int32_t>(0));
    const castle::units::fahrenheit<int32_t> boiling(castle::units::celsius<int32_t>(100));
    CASTLE_SAMPLE_CHECK(coarse.value() == 273);
    CASTLE_SAMPLE_CHECK(boiling.value() == 212);
    CASTLE_SAMPLE_CHECK(-40_degC == -40_degF);

    // Same-unit sums are fine; differences between temperatures go through kelvin.
    const castle::units::celsius<int64_t> warmer = 20_degC + 5_degC;
    CASTLE_SAMPLE_CHECK(warmer.value() == 25);
    const castle::units::kelvin<double> delta = castle::units::kelvin<double>(castle::units::celsius<double>(30.0)) -
                                                castle::units::kelvin<double>(castle::units::celsius<double>(20.0));
    CASTLE_SAMPLE_CHECK(delta.value() > 9.999 && delta.value() < 10.001);
    return 0;
}
