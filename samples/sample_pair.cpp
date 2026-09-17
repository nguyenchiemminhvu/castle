#include "sample_support.h"

#include "castle/utility/pair.h"

// Scenario: storing a compact key/value record used by deterministic lookup tables.
int main()
{
    auto item = castle::make_pair(7U, 25U);
    CASTLE_SAMPLE_CHECK(item.first == 7U);
    CASTLE_SAMPLE_CHECK(item.second == 25U);
    auto other = castle::make_pair(1U, 2U);
    item.swap(other);
    CASTLE_SAMPLE_CHECK(item.first == 1U);
    return 0;
}
