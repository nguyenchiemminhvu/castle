#include "sample_support.hpp"

#include "castle/math/lcm.hpp"

// Scenario: using lcm in a deterministic control/telemetry calculation.
int main()
{
    CASTLE_SAMPLE_CHECK((castle::math::lcm<12U, 15U>::value == 60U));
    return 0;
}
