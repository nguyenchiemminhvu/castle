#include "sample_support.h"

#include "castle/iterator/operations.h"

// Scenario: generic iterator traversal in fixed buffers without depending on the STL algorithms header.
int main()
{
    uint8_t data[5U] = {1U, 2U, 3U, 4U, 5U};
    auto first = data;
    auto last = data + 5U;
    CASTLE_SAMPLE_CHECK(castle::distance(first, last) == 5);
    auto third = castle::next(first, 2);
    CASTLE_SAMPLE_CHECK(*third == 3U);
    auto fourth = castle::prev(last, 2);
    CASTLE_SAMPLE_CHECK(*fourth == 4U);
    return 0;
}
