#include "sample_support.hpp"

#include "castle/units/current.hpp"
#include "castle/units/resistance.hpp"
#include "castle/units/voltage.hpp"

#include <stdint.h>

namespace
{

// Current-sense amplifier driver: the shunt value and amplifier output are typed,
// so Ohm's law (I = V / R) yields amperes without manual scaling.
class current_sensor
{
public:
    explicit current_sensor(castle::units::ohms<double> shunt) : shunt_(shunt)
    {
    }

    castle::units::amperes<double> current(castle::units::volts<double> shunt_drop) const
    {
        return shunt_drop / shunt_;
    }

private:
    castle::units::ohms<double> shunt_;
};

} // namespace

int main()
{
    using namespace castle::units::literals;

    const current_sensor sensor(10.0_mOhm);
    const castle::units::amperes<double> load = sensor.current(25.0_mV);
    CASTLE_SAMPLE_CHECK(load.value() > 2.4999 && load.value() < 2.5001);

    // R = V / I and V = R * I produce the coherent units directly.
    const castle::units::ohms<double> pullup = 3.3_V / 0.33_mA;
    CASTLE_SAMPLE_CHECK(pullup.value() > 9999.9 && pullup.value() < 10000.1);
    const castle::units::volts<double> drop = 4.7_kOhm * 2.0_mA;
    CASTLE_SAMPLE_CHECK(drop.value() > 9.3999 && drop.value() < 9.4001);

    const castle::units::ohms<int64_t> divider = 4_kOhm + 700_Ohm;
    CASTLE_SAMPLE_CHECK(divider.value() == 4700);
    const castle::units::kiloohms<int32_t> coarse(castle::units::ohms<int32_t>(4700));
    CASTLE_SAMPLE_CHECK(coarse.value() == 4);
    CASTLE_SAMPLE_CHECK(1_MOhm == 1000_kOhm);
    return 0;
}
