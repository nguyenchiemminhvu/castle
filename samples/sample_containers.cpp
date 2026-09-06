#include "sample_support.h"

#include "castle/container/containers.h"

// Scenario: umbrella-header smoke test for the full fixed-capacity container set.
int main()
{
    // The umbrella header is intended to expose the container family through one
    // include; keep a representative concrete use here.
    castle::container::array<uint32_t, 2U> bytes{0x12U, 0x34U};
    CASTLE_SAMPLE_CHECK(bytes[1U] == 0x34U);
    return 0;
}
