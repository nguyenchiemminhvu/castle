#include "sample_support.h"

#include "castle/chrono/time_point.h"

// Scenario: computing deterministic deadlines from a monotonic timestamp representation.
#include <stdint.h>

namespace
{
struct test_clock
{
    using duration = castle::chrono::milliseconds;
};
} // namespace

int main()
{
    using point = castle::chrono::time_point<test_clock, castle::chrono::milliseconds>;
    point start(castle::chrono::milliseconds(100));
    point deadline = start + castle::chrono::milliseconds(25);
    CASTLE_SAMPLE_CHECK((deadline - start).count() == 25);
    CASTLE_SAMPLE_CHECK(deadline.time_since_epoch().count() == 125);
    return 0;
}
