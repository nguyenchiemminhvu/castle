#include "sample_support.hpp"

#include "castle/math/fib.hpp"

// Scenario: using fib in a deterministic control/telemetry calculation.
int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::fib(10U) == 55U);
    return 0;
}
