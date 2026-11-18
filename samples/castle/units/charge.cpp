#include "sample_support.hpp"

#include "castle/units/charge.hpp"
#include "castle/units/time.hpp"

#include <stdint.h>

namespace
{

using ampere_unit = castle::units::unit<castle::units::dim::current>;

// Coulomb counter: integrates sampled current into the remaining battery charge.
// The parameter and return types document and enforce the unit of each value.
class battery_gauge
{
public:
    explicit battery_gauge(castle::units::milliampere_hours<double> capacity)
        : remaining_(capacity)
    {
    }

    void sample(castle::units::quantity<ampere_unit, double> discharge_current,
                castle::units::seconds<double> period)
    {
        const castle::units::coulombs<double> used = discharge_current * period;
        remaining_ = remaining_ - used;
    }

    castle::units::milliampere_hours<double> remaining() const
    {
        return remaining_;
    }

private:
    castle::units::milliampere_hours<double> remaining_;
};

} // namespace

int main()
{
    using namespace castle::units::literals;

    battery_gauge gauge(2000.0_mAh);

    // 0.5 A for one hour, sampled once per minute, uses 500 mAh.
    for (int32_t i = 0; i < 60; ++i)
    {
        gauge.sample(castle::units::quantity<ampere_unit, double>(0.5), 60.0_s);
    }
    const double left = gauge.remaining().value();
    CASTLE_SAMPLE_CHECK(left > 1499.999 && left < 1500.001);

    // Exact integral conversions between capacity units.
    const castle::units::milliampere_hours<int32_t> pack = castle::units::ampere_hours<int32_t>(3);
    CASTLE_SAMPLE_CHECK(pack.value() == 3000);
    CASTLE_SAMPLE_CHECK(1_Ah == 1000_mAh);
    const castle::units::coulombs<double> one_mah = 1.0_mAh;
    CASTLE_SAMPLE_CHECK(one_mah.value() > 3.5999 && one_mah.value() < 3.6001);
    return 0;
}
