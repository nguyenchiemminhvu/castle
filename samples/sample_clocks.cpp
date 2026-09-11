#include "sample_support.h"

#include "castle/chrono/clocks.h"

// Scenario: obtaining a monotonic scheduler timestamp from the Castle clock abstraction.
int main()
{
    // The clock APIs are consumed by the board-specific clock implementation.
    // Keep the sample side-effect free so it remains usable in a bare-metal test.
    const auto now = castle::chrono::steady_clock::now();
    (void)now;
    return 0;
}
