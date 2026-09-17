#include "sample_support.hpp"

#include "castle/container/hash_map.hpp"

int main()
{
    castle::container::hash_map<uint8_t, uint16_t, 3U> map;
    CASTLE_SAMPLE_CHECK(map.capacity() == 3U);
    CASTLE_SAMPLE_CHECK(map.empty());

    CASTLE_SAMPLE_CHECK(map.insert(1U, 100U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.insert(2U, 200U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.insert(1U, 111U) == castle::status::already_exists);
    CASTLE_SAMPLE_CHECK(map.try_emplace(3U, 300U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.full());
    CASTLE_SAMPLE_CHECK(map.try_emplace(4U, 400U) == castle::status::full);

    CASTLE_SAMPLE_CHECK(map.contains(2U));
    CASTLE_SAMPLE_CHECK(map.get(3U) != nullptr);
    CASTLE_SAMPLE_CHECK(*map.get(3U) == 300U);

    auto it = map.find(2U);
    CASTLE_SAMPLE_CHECK(it != map.end());
    CASTLE_SAMPLE_CHECK(it->first == 2U);
    CASTLE_SAMPLE_CHECK(it->second == 200U);

    CASTLE_SAMPLE_CHECK(map.insert_or_assign(2U, 250U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(*map.get(2U) == 250U);

    CASTLE_SAMPLE_CHECK(map.erase(1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(map.erase(9U) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(!map.contains(1U));
    CASTLE_SAMPLE_CHECK(map.size() == 2U);

    size_t count = 0U;
    for (auto iter = map.begin(); iter != map.end(); ++iter)
    {
        ++count;
    }
    CASTLE_SAMPLE_CHECK(count == 2U);

    map.clear();
    CASTLE_SAMPLE_CHECK(map.empty());
    CASTLE_SAMPLE_CHECK(map.begin() == map.end());

    return 0;
}
