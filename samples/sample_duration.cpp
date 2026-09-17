#include "sample_support.h"

#include "castle/chrono/duration.h"

// Scenario: representing fixed scheduler and telemetry periods with integer duration arithmetic.
#include <stdint.h>

int main()
{
    castle::chrono::milliseconds sample_period(10);
    castle::chrono::microseconds finer = castle::chrono::duration_cast<castle::chrono::microseconds>(sample_period);
    CASTLE_SAMPLE_CHECK(finer.count() == 10000);

    const auto total = sample_period + castle::chrono::milliseconds(5);
    CASTLE_SAMPLE_CHECK(total.count() == 15);
    const auto scaled = sample_period * 2;
    CASTLE_SAMPLE_CHECK(scaled.count() == 20);
    return 0;
}
