#include "sample_support.hpp"

#include "castle/math/factorial.hpp"

// Scenario: using factorial in a deterministic control/telemetry calculation.
int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::factorial<5U>::value == 120U);
    CASTLE_SAMPLE_CHECK(castle::math::factorial_v(5U) == 120U);
    return 0;
}
