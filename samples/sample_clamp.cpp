#include "sample_support.h"

#include "castle/math/clamp.h"

// Scenario: using clamp in a deterministic control/telemetry calculation.
int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::clamp(150, 0, 100) == 100);
    CASTLE_SAMPLE_CHECK(castle::math::clamp(-1, 0, 100) == 0);
    return 0;
}
