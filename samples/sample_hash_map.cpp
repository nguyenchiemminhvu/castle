#include "sample_support.h"

#include "castle/container/hash_map.h"

// Scenario: fixed-capacity lookup of sensor IDs to scaled sensor values.
int main()
{
    castle::container::hash_map<uint8_t, uint16_t, 8U> map;
    CASTLE_SAMPLE_CHECK(map.insert(1U, 100U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.contains(1U));
    CASTLE_SAMPLE_CHECK(*map.get(1U) == 100U);
    return 0;
}
