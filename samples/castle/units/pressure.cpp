#include "sample_support.hpp"

#include "castle/units/pressure.hpp"

#include <stdint.h>

namespace
{

// Barometric sensor driver: the raw register is in Pa / 4 (fixed point), the
// API returns typed pascals.
castle::units::pascals<int32_t> baro_read(uint32_t raw_quarter_pa)
{
    return castle::units::pascals<int32_t>(static_cast<int32_t>(raw_quarter_pa / 4U));
}

// Tyre-pressure monitor API in kilopascals; any pressure unit converts to it.
bool tyre_underinflated(castle::units::kilopascals<double> measured)
{
    return measured < castle::units::kilopascals<double>(200.0);
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    const castle::units::pascals<int32_t> station = baro_read(405300U);
    CASTLE_SAMPLE_CHECK(station.value() == 101325);
    CASTLE_SAMPLE_CHECK(station == 1_atm);

    // Hectopascals and millibars are the same unit.
    const castle::units::hectopascals<double> qnh = station;
    CASTLE_SAMPLE_CHECK(qnh.value() == 1013.25);
    CASTLE_SAMPLE_CHECK(1013_hPa == 1013_mbar);

    // Imperial gauges convert with the exact psi definition.
    CASTLE_SAMPLE_CHECK(tyre_underinflated(28.0_psi));
    CASTLE_SAMPLE_CHECK(!tyre_underinflated(2.4_bar));
    const castle::units::kilopascals<double> tyre = 32.0_psi;
    CASTLE_SAMPLE_CHECK(tyre.value() > 220.632 && tyre.value() < 220.633);

    // Exact integral conversion is implicit; truncating needs an explicit step.
    const castle::units::pascals<int64_t> line = 2_bar + 150_hPa;
    CASTLE_SAMPLE_CHECK(line.value() == 215000);
    const castle::units::bars<int64_t> coarse(line);
    CASTLE_SAMPLE_CHECK(coarse.value() == 2);
    CASTLE_SAMPLE_CHECK(1_MPa == 10_bar);
    return 0;
}
