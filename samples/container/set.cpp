#include "sample_support.hpp"

#include "castle/container/set.hpp"

int main()
{
    castle::container::set<uint16_t, 3U> active_channels{4U, 2U, 4U};
    CASTLE_SAMPLE_CHECK(active_channels.size() == 2U);
    CASTLE_SAMPLE_CHECK(active_channels.begin() != active_channels.end());
    CASTLE_SAMPLE_CHECK(*active_channels.begin() == 2U);

    CASTLE_SAMPLE_CHECK(active_channels.insert(3U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(active_channels.full());
    CASTLE_SAMPLE_CHECK(active_channels.insert(1U) == castle::status::full);

    auto found = active_channels.find(3U);
    CASTLE_SAMPLE_CHECK(found != active_channels.end());
    CASTLE_SAMPLE_CHECK(*found == 3U);
    CASTLE_SAMPLE_CHECK(active_channels.contains(4U));

    auto lower = active_channels.lower_bound(3U);
    CASTLE_SAMPLE_CHECK(lower != active_channels.end());
    CASTLE_SAMPLE_CHECK(*lower == 3U);

    auto upper = active_channels.upper_bound(3U);
    CASTLE_SAMPLE_CHECK(upper != active_channels.end());
    CASTLE_SAMPLE_CHECK(*upper == 4U);
    CASTLE_SAMPLE_CHECK(active_channels.lower_bound(9U) == active_channels.end());

    CASTLE_SAMPLE_CHECK(active_channels.erase(2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(active_channels.erase(2U) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(!active_channels.contains(2U));

    active_channels.clear();
    CASTLE_SAMPLE_CHECK(active_channels.empty());

    return 0;
}
