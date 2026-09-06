#include "sample_support.h"

#include "castle/chrono/literals.h"

// Scenario: expressing timer constants in readable source without runtime conversion.
int main()
{
    using namespace castle::chrono::literals::chrono_literals;
    const auto period = 10_ms;
    CASTLE_SAMPLE_CHECK(period.count() == 10);
    return 0;
}
