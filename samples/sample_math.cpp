#include "sample_support.hpp"

#include "castle/math/math.hpp"

// Scenario: using math in a deterministic control/telemetry calculation.
int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::abs(-7) == 7);
    return 0;
}
