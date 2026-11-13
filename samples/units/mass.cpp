#include "sample_support.hpp"

#include "castle/units/mass.hpp"

#include <stdint.h>

namespace
{

// Load-cell driver: the parameter type fixes the unit of the tare offset, so a
// caller cannot pass grams where kilograms are expected without a conversion.
castle::units::kilograms<double> net_weight(castle::units::kilograms<double> gross,
                                            castle::units::kilograms<double> tare)
{
    return gross - tare;
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    const castle::units::kilograms<double> net = net_weight(12.5_kg, 250.0_g);
    CASTLE_SAMPLE_CHECK(net.value() == 12.25);

    // The kilogram is the coherent unit; exact integral conversion is implicit.
    const castle::units::grams<int32_t> cargo = castle::units::kilograms<int32_t>(3);
    CASTLE_SAMPLE_CHECK(cargo.value() == 3000);
    const castle::units::kilograms<int32_t> rounded(castle::units::grams<int32_t>(3999));
    CASTLE_SAMPLE_CHECK(rounded.value() == 3);

    // Imperial units use exact international definitions.
    CASTLE_SAMPLE_CHECK(16_oz == 1_lb);
    CASTLE_SAMPLE_CHECK(1_t == 1000_kg);
    const castle::units::kilograms<double> pound = 1.0_lb;
    CASTLE_SAMPLE_CHECK(pound.value() > 0.45359 && pound.value() < 0.45360);
    return 0;
}
