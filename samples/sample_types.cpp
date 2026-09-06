#include "sample_support.h"

#include "castle/core/types.h"

// Scenario: using Castle’s target-stable size/difference typedefs in embedded APIs.
int main()
{
    castle::size_type count = 4U;
    castle::difference_type delta = -1;
    CASTLE_SAMPLE_CHECK(count == 4U);
    CASTLE_SAMPLE_CHECK(delta < 0);
    return 0;
}
