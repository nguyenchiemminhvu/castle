#include "sample_support.hpp"

#include "castle/math/logarithm.hpp"

// Scenario: using logarithm in a deterministic control/telemetry calculation.
int main()
{
    CASTLE_SAMPLE_CHECK((castle::math::logarithm<1024U, 2U>::value == 10U));
    return 0;
}
