#include "sample_support.h"

#include "castle/math/invert.h"

// Scenario: using invert in a deterministic control/telemetry calculation.
int main()
{
    const float value = castle::math::invert<float>().operator()(4.0f);
    CASTLE_SAMPLE_CHECK(value > 0.249f && value < 0.251f);
    return 0;
}
