#include "sample_support.h"

#include "castle/math/abs.h"

// Scenario: using abs in a deterministic control/telemetry calculation.
int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::abs(-42) == 42);
    return 0;
}
