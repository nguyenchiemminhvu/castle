#include "sample_support.h"

#include "castle/utility/swap.h"

// Scenario: swapping fixed objects using Castle’s noexcept-aware utility.
int main()
{
    uint32_t a = 1U, b = 2U;
    castle::swap(a,b);
    CASTLE_SAMPLE_CHECK(a == 2U && b == 1U);
    return 0;
}
