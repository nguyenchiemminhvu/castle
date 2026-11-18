#include "sample_support.hpp"

#include "castle/units/current.hpp"

#include <stdint.h>

namespace
{

// A motor driver with a typed over-current limit; callers may pass any current
// unit that converts losslessly to integral milliamperes.
class motor_driver
{
public:
    explicit motor_driver(castle::units::milliamperes<int32_t> limit) : limit_(limit)
    {
    }

    bool accept(castle::units::milliamperes<int32_t> measured) const
    {
        return measured <= limit_;
    }

private:
    castle::units::milliamperes<int32_t> limit_;
};

} // namespace

int main()
{
    using namespace castle::units::literals;

    const motor_driver driver(castle::units::amperes<int32_t>(2));
    CASTLE_SAMPLE_CHECK(driver.accept(castle::units::milliamperes<int32_t>(1500)));
    CASTLE_SAMPLE_CHECK(!driver.accept(castle::units::milliamperes<int32_t>(2001)));

    const castle::units::milliamperes<int64_t> load = 1_A + 250_mA;
    CASTLE_SAMPLE_CHECK(load.value() == 1250);

    // Exact integral conversion is implicit; truncating needs an explicit step.
    const castle::units::microamperes<int32_t> leak = castle::units::milliamperes<int32_t>(3);
    const castle::units::milliamperes<int32_t> coarse(castle::units::microamperes<int32_t>(3999));
    CASTLE_SAMPLE_CHECK(leak.value() == 3000);
    CASTLE_SAMPLE_CHECK(coarse.value() == 3);

    const castle::units::amperes<double> peak = 1500.0_mA;
    CASTLE_SAMPLE_CHECK(peak.value() == 1.5);
    CASTLE_SAMPLE_CHECK(1_kA == 1000_A);
    return 0;
}
