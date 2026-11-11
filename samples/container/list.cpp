#include "sample_support.hpp"

#include "castle/container/list.hpp"

#include <stdint.h>

namespace
{
using list_type = castle::container::list<uint32_t, 4U>;

struct pair_value
{
    pair_value(uint32_t k, uint32_t v)
        : key(k), value(v) {}
    uint32_t key;
    uint32_t value;
};

bool order_is(const list_type& values, const uint32_t* expected, uint32_t count)
{
    if (values.size() != count)
    {
        return false;
    }
    uint32_t index = 0U;
    for (list_type::const_iterator it = values.begin(); it != values.end(); ++it)
    {
        if (*it != expected[index])
        {
            return false;
        }
        ++index;
    }
    return true;
}
} // namespace

int main()
{
    // Deque-style use: both ends.
    list_type values;
    CASTLE_SAMPLE_CHECK(values.capacity() == 4U);
    CASTLE_SAMPLE_CHECK(values.empty());
    CASTLE_SAMPLE_CHECK(values.push_back(2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.push_back(3U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.push_front(1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.emplace_front(0U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.full());
    CASTLE_SAMPLE_CHECK(values.push_back(9U) == castle::status::full);
    CASTLE_SAMPLE_CHECK(values.front() == 0U);
    CASTLE_SAMPLE_CHECK(values.back() == 3U);

    const uint32_t forward_order[] = {0U, 1U, 2U, 3U};
    CASTLE_SAMPLE_CHECK(order_is(values, forward_order, 4U));

    // Reverse traversal.
    uint32_t expected_back = 3U;
    for (list_type::reverse_iterator it = values.rbegin(); it != values.rend(); ++it)
    {
        CASTLE_SAMPLE_CHECK(*it == expected_back);
        --expected_back;
    }

    CASTLE_SAMPLE_CHECK(values.pop_front() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.pop_back() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.size() == 2U);

    // Insert / erase at a known position.
    list_type::iterator two = values.find(2U);
    CASTLE_SAMPLE_CHECK(two != values.end());
    list_type::iterator inserted = values.insert(two, 7U);
    CASTLE_SAMPLE_CHECK(*inserted == 7U);
    CASTLE_SAMPLE_CHECK(*values.erase(inserted) == 2U);
    CASTLE_SAMPLE_CHECK(values.remove(2U) == 1U);
    CASTLE_SAMPLE_CHECK(values.size() == 1U);
    values.clear();
    CASTLE_SAMPLE_CHECK(values.empty());

    // LRU recency list: front = most recently used, back = eviction candidate.
    list_type recency{10U, 20U, 30U};
    list_type::iterator touched = recency.find(30U);
    recency.move_to_front(touched);
    const uint32_t after_touch[] = {30U, 10U, 20U};
    CASTLE_SAMPLE_CHECK(order_is(recency, after_touch, 3U));
    CASTLE_SAMPLE_CHECK(*touched == 30U);
    CASTLE_SAMPLE_CHECK(recency.back() == 20U);
    CASTLE_SAMPLE_CHECK(recency.pop_back() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(recency.push_front(40U) == castle::status::ok);
    const uint32_t after_evict[] = {40U, 30U, 10U};
    CASTLE_SAMPLE_CHECK(order_is(recency, after_evict, 3U));

    // In-place element construction and relocation to the back.
    castle::container::list<pair_value, 2U> entries;
    CASTLE_SAMPLE_CHECK(entries.emplace_back(1U, 100U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(entries.emplace_back(2U, 200U) == castle::status::ok);
    entries.move_to_back(entries.begin());
    CASTLE_SAMPLE_CHECK(entries.front().key == 2U);
    CASTLE_SAMPLE_CHECK(entries.back().value == 100U);
    entries.reverse();
    CASTLE_SAMPLE_CHECK(entries.front().key == 1U);
    return 0;
}
