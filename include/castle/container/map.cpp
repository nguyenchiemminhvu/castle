#include "sample_support.hpp"

#include "castle/container/map.hpp"

int main()
{
    castle::container::map<uint8_t, uint16_t, 3U> map;
    CASTLE_SAMPLE_CHECK(map.capacity() == 3U);
    CASTLE_SAMPLE_CHECK(map.empty());

    CASTLE_SAMPLE_CHECK(map.insert(2U, 200U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.insert(1U, 100U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.insert(2U, 250U) == castle::status::already_exists);
    CASTLE_SAMPLE_CHECK(map.try_emplace(3U, 300U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.full());
    CASTLE_SAMPLE_CHECK(map.try_emplace(4U, 400U) == castle::status::full);

    CASTLE_SAMPLE_CHECK(map.begin()->first == 1U);
    CASTLE_SAMPLE_CHECK(map.contains(2U));
    CASTLE_SAMPLE_CHECK(map.get(1U) != nullptr);
    CASTLE_SAMPLE_CHECK(*map.get(1U) == 100U);

    auto found = map.find(2U);
    CASTLE_SAMPLE_CHECK(found != map.end());
    CASTLE_SAMPLE_CHECK(found->second == 200U);

    auto lower = map.lower_bound(2U);
    CASTLE_SAMPLE_CHECK(lower != map.end());
    CASTLE_SAMPLE_CHECK(lower->first == 2U);

    auto upper = map.upper_bound(2U);
    CASTLE_SAMPLE_CHECK(upper != map.end());
    CASTLE_SAMPLE_CHECK(upper->first == 3U);
    CASTLE_SAMPLE_CHECK(map.upper_bound(3U) == map.end());

    auto next = map.erase(map.find(1U));
    CASTLE_SAMPLE_CHECK(next != map.end());
    CASTLE_SAMPLE_CHECK(next->first == 2U);
    CASTLE_SAMPLE_CHECK(!map.contains(1U));

    CASTLE_SAMPLE_CHECK(map.erase(9U) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(map.erase(2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.size() == 1U);

    map.clear();
    CASTLE_SAMPLE_CHECK(map.empty());

    return 0;
}
