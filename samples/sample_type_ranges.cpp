#include "sample_support.h"

#include "castle/core/type_ranges.h"

// Scenario: range checks used to validate sensor scaling without including <limits>.
int main()
{
    CASTLE_SAMPLE_CHECK(castle::numeric_limits<uint32_t>::max() > 1000U);
    CASTLE_SAMPLE_CHECK(castle::numeric_limits<int32_t>::lowest() < 0);
    return 0;
}
