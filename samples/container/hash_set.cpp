#include "sample_support.hpp"

#include "castle/container/hash_set.hpp"

int main()
{
    castle::container::hash_set<uint8_t, 3U> set{1U, 2U, 2U};
    CASTLE_SAMPLE_CHECK(set.size() == 2U);
    CASTLE_SAMPLE_CHECK(set.insert(2U) == castle::status::already_exists);
    CASTLE_SAMPLE_CHECK(set.insert(3U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(set.full());
    CASTLE_SAMPLE_CHECK(set.insert(4U) == castle::status::full);

    auto it = set.find(1U);
    CASTLE_SAMPLE_CHECK(it != set.end());
    CASTLE_SAMPLE_CHECK(*it == 1U);
    CASTLE_SAMPLE_CHECK(set.contains(3U));

    CASTLE_SAMPLE_CHECK(set.erase(2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(set.erase(2U) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(!set.contains(2U));

    size_t count = 0U;
    for (auto iter = set.begin(); iter != set.end(); ++iter)
    {
        ++count;
    }
    CASTLE_SAMPLE_CHECK(count == 2U);

    set.clear();
    CASTLE_SAMPLE_CHECK(set.empty());
    CASTLE_SAMPLE_CHECK(set.begin() == set.end());

    return 0;
}
