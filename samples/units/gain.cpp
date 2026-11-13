#include "sample_support.hpp"

#include "castle/units/gain.hpp"

#include <stdint.h>

namespace
{

// RF front end: the gain of each stage is a typed quantity, so a stage gain
// can never be confused with a plain ratio or a different dimension.
struct receive_chain
{
    castle::units::decibels<double> antenna_gain;
    castle::units::decibels<double> amplifier_gain;
    castle::units::decibels<double> cable_loss;
};

castle::units::decibels<double> total_gain(const receive_chain& chain)
{
    return chain.antenna_gain + chain.amplifier_gain - chain.cable_loss;
}

// The driver takes a typed gain; its limit check is expressed in bels or decibels alike.
bool set_amplifier_gain(castle::units::decibels<float> requested, castle::units::decibels<float>& applied)
{
    if (requested > castle::units::decibels<float>(30.0F))
    {
        return false;
    }
    applied = requested;
    return true;
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    // Cascaded stages add in decibels.
    const receive_chain chain = {3.0_dB, 20.0_dB, 2.0_dB};
    const castle::units::decibels<double> total = total_gain(chain);
    CASTLE_SAMPLE_CHECK(total.value() == 21.0);

    // Convert to linear ratios only where a multiplier is needed.
    const double power = castle::units::power_ratio(20.0_dB);
    const double amplitude = castle::units::amplitude_ratio(20.0_dB);
    CASTLE_SAMPLE_CHECK(power > 99.999 && power < 100.001);
    CASTLE_SAMPLE_CHECK(amplitude > 9.9999 && amplitude < 10.0001);

    // Measured ratios become typed gains.
    const castle::units::decibels<double> measured = castle::units::decibels_from_amplitude_ratio(10.0);
    CASTLE_SAMPLE_CHECK(measured.value() > 19.999 && measured.value() < 20.001);

    // Bels and decibels convert exactly for integers.
    const castle::units::decibels<int32_t> twenty = castle::units::bels<int32_t>(2);
    CASTLE_SAMPLE_CHECK(twenty.value() == 20);

    castle::units::decibels<float> applied(0.0F);
    CASTLE_SAMPLE_CHECK(set_amplifier_gain(castle::units::decibels<float>(25.0F), applied));
    CASTLE_SAMPLE_CHECK(applied.value() == 25.0F);
    CASTLE_SAMPLE_CHECK(!set_amplifier_gain(castle::units::decibels<float>(45.0F), applied));
    CASTLE_SAMPLE_CHECK(applied.value() == 25.0F);
    return 0;
}
