#include "sample_support.hpp"

#include "castle/math/abs.hpp"

// Scenario: using abs in a deterministic control/telemetry calculation.
int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::abs(-42) == 42);
    return 0;
}
