#include "sample_support.h"

#include "castle/utility/compare.h"

// Scenario: ordering command IDs and calibration keys with a single canonical comparator.
int main()
{
    CASTLE_SAMPLE_CHECK(castle::less<int>()(1,2));
    CASTLE_SAMPLE_CHECK(castle::greater<int>()(2,1));
    CASTLE_SAMPLE_CHECK(castle::equal_to<int>()(4,4));
    CASTLE_SAMPLE_CHECK(castle::compare<int>::lt(1,2));
    CASTLE_SAMPLE_CHECK(castle::compare<int>::gte(2,2));
    return 0;
}
