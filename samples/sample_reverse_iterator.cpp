#include "sample_support.h"

#include "castle/iterator/reverse_iterator.h"

// Scenario: walking a fixed log buffer newest-entry first.
int main()
{
    uint8_t data[3U] = {1U, 2U, 3U};
    castle::reverse_iterator<uint8_t*> it(data + 3U);
    CASTLE_SAMPLE_CHECK(*it == 3U);
    ++it;
    CASTLE_SAMPLE_CHECK(*it == 2U);
    return 0;
}
