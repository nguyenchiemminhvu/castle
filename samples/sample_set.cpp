#include "sample_support.hpp"

#include "castle/container/set.hpp"

// Scenario: maintaining a sorted set of active channel IDs without heap allocation.
int main()
{
    castle::container::set<uint16_t, 8U> active_channels;
    CASTLE_SAMPLE_CHECK(active_channels.insert(4U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(active_channels.insert(8U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(active_channels.contains(8U));
    return 0;
}
