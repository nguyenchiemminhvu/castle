#include "sample_support.h"

#include "castle/math/gcd.h"

// Scenario: using gcd in a deterministic control/telemetry calculation.
int main()
{
    CASTLE_SAMPLE_CHECK((castle::math::gcd<84U, 30U>::value == 6U));
    return 0;
}
