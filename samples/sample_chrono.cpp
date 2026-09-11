#include "sample_support.h"

#include "castle/chrono/chrono.h"

// Scenario: umbrella-header smoke test for the complete Castle chrono facility.
int main()
{
    castle::chrono::milliseconds watchdog_window(250);
    CASTLE_SAMPLE_CHECK(watchdog_window.count() == 250);
    return 0;
}
