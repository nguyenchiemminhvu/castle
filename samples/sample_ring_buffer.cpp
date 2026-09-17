#include "sample_support.h"

#include "castle/container/ring_buffer.h"

// Scenario: retaining the most recent telemetry samples while bounding RAM exactly.
int main()
{
    castle::container::ring_buffer<uint16_t, 4U> buffer;
    CASTLE_SAMPLE_CHECK(buffer.push(1U));
    CASTLE_SAMPLE_CHECK(buffer.push(2U));
    CASTLE_SAMPLE_CHECK(buffer.push(3U));
    CASTLE_SAMPLE_CHECK(buffer.front() == 1U);
    CASTLE_SAMPLE_CHECK(buffer.pop() == 1U);
    CASTLE_SAMPLE_CHECK(buffer.force_push(4U) == false);
    buffer.force_push(5U);
    CASTLE_SAMPLE_CHECK(buffer.back() == 5U);
    return 0;
}
