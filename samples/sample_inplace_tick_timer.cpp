#include "sample_support.h"

#include "castle/events/inplace_tick_timer.h"

// Scenario: deterministic tick-driven timeout handling.
int main()
{
    // The timer is advanced by the board tick ISR. Keep the example intentionally
    // independent from a system clock or thread scheduler.
    return 0;
}
