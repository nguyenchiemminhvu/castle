#include "sample_support.h"

#include "castle/iterator/fixed_iterator.h"

// Scenario: preventing accidental traversal beyond a validated fixed buffer range.
int main()
{
    uint8_t data[4U] = {1U, 2U, 3U, 4U};
    castle::fixed_iterator<uint8_t*> it(data, data, data + 4U);
    ++it;
    ++it;
    CASTLE_SAMPLE_CHECK(*it == 3U);
    while (it.valid())
    {
        ++it;
    }
    CASTLE_SAMPLE_CHECK(!it.valid());
    return 0;
}
