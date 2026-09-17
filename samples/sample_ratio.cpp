#include "sample_support.h"

#include "castle/math/ratio.h"

// Scenario: using ratio in a deterministic control/telemetry calculation.
int main()
{
    using rate = castle::math::ratio<1000, 1>;
    static_assert(rate::num == 1000 && rate::den == 1, "ratio");
    return 0;
}
