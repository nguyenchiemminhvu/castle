#include "sample_support.hpp"

#include "castle/math/mean.hpp"

// Scenario: using mean in a deterministic control/telemetry calculation.
int main()
{
    castle::math::mean<int> m;
    m.add(10);
    m.add(20);
    m.add(30);
    CASTLE_SAMPLE_CHECK(m.get_mean() == 20.0);
    return 0;
}
