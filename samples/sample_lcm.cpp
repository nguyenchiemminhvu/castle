#include "sample_support.h"

#include "castle/math/lcm.h"

// Scenario: using lcm in a deterministic control/telemetry calculation.
int main()
{
    CASTLE_SAMPLE_CHECK((castle::math::lcm<12U, 15U>::value == 60U));
    return 0;
}
