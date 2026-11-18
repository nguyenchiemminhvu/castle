#include "sample_support.hpp"

#include "castle/units/voltage.hpp"

#include <stdint.h>

namespace
{

// A 12-bit ADC driver that reports its reading as millivolts rather than raw counts.
castle::units::millivolts<int32_t> adc_read(uint16_t raw_counts, castle::units::millivolts<int32_t> reference)
{
    return castle::units::millivolts<int32_t>((static_cast<int32_t>(raw_counts) * reference.value()) / 4095);
}

// Battery monitor API expressed in volts; millivolts convert to it implicitly.
bool battery_low(castle::units::volts<double> cell)
{
    return cell < castle::units::volts<double>(3.3);
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    const castle::units::millivolts<int32_t> vref = castle::units::volts<int32_t>(3);
    CASTLE_SAMPLE_CHECK(vref.value() == 3000);

    const castle::units::millivolts<int32_t> reading = adc_read(2730U, vref);
    CASTLE_SAMPLE_CHECK(reading.value() == 2000);

    // mV -> V is lossless into double, so it is implicit.
    const castle::units::volts<double> volts = reading;
    CASTLE_SAMPLE_CHECK(volts.value() == 2.0);
    CASTLE_SAMPLE_CHECK(battery_low(reading));
    CASTLE_SAMPLE_CHECK(!battery_low(4.2_V));

    // Truncating to whole volts must be explicit.
    const castle::units::volts<int32_t> whole(castle::units::millivolts<int32_t>(3300));
    CASTLE_SAMPLE_CHECK(whole.value() == 3);

    const castle::units::millivolts<int64_t> rail = 3_V + 300_mV;
    CASTLE_SAMPLE_CHECK(rail.value() == 3300);
    CASTLE_SAMPLE_CHECK(1_mV == 1000_uV);
    CASTLE_SAMPLE_CHECK(1_kV == 1000_V);
    return 0;
}
