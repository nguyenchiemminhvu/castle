#include "sample_support.h"

#include "castle/container/hash_set.h"

// Scenario: tracking a bounded set of enabled peripheral channels.
int main()
{
    castle::container::hash_set<uint8_t, 8U> set;
    CASTLE_SAMPLE_CHECK(set.insert(3U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(set.contains(3U));
    CASTLE_SAMPLE_CHECK(set.erase(3U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!set.contains(3U));
    return 0;
}
