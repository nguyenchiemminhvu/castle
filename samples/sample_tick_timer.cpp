#include "sample_support.hpp"

#include "castle/events/tick_timer.hpp"

// Scenario: deterministic tick-driven timeout handling.
int main()
{
    // The timer is advanced by the board tick ISR. Keep the example intentionally
    // independent from a system clock or thread scheduler.
    return 0;
}
