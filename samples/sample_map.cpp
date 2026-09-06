#include "sample_support.h"

#include "castle/container/map.h"

// Scenario: ordered deterministic calibration mapping with bounded AVL storage.
int main()
{
    castle::container::map<uint8_t, uint16_t, 8U> map;
    CASTLE_SAMPLE_CHECK(map.insert(2U, 200U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.try_emplace(4U, 400U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.contains(2U));
    auto it = map.find(4U);
    CASTLE_SAMPLE_CHECK(it != map.end() && it->second == 400U);
    return 0;
}
