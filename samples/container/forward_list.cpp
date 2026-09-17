#include "sample_support.hpp"

#include "castle/container/forward_list.hpp"

#include <stdint.h>

int main()
{
    castle::container::forward_list<uint32_t, 4U> values;
    CASTLE_SAMPLE_CHECK(values.capacity() == 4U);
    CASTLE_SAMPLE_CHECK(values.max_size() == 4U);
    CASTLE_SAMPLE_CHECK(values.available() == 4U);
    CASTLE_SAMPLE_CHECK(values.empty());

    uint32_t copy_value = 2U;
    CASTLE_SAMPLE_CHECK(values.push_front(copy_value) == castle::status::ok);

    uint32_t moved_value = 3U;
    CASTLE_SAMPLE_CHECK(values.push_front(castle::move(moved_value)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.emplace_front(1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.front() == 1U);

    auto before = values.before_begin();
    CASTLE_SAMPLE_CHECK(values.emplace_after(before, 0U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.full());
    CASTLE_SAMPLE_CHECK(values.front() == 0U);
    CASTLE_SAMPLE_CHECK(values.emplace_after(values.begin(), 99U) == castle::status::full);

    auto it = values.begin();
    CASTLE_SAMPLE_CHECK(*it == 0U);
    ++it;
    CASTLE_SAMPLE_CHECK(*it == 1U);

    const castle::container::forward_list<uint32_t, 4U>& cvalues = values;
    auto cit = cvalues.before_begin();
    ++cit;
    CASTLE_SAMPLE_CHECK(*cit == 0U);
    CASTLE_SAMPLE_CHECK(cvalues.find(2U) != cvalues.cend());

    CASTLE_SAMPLE_CHECK(values.find(3U) != values.end());
    auto erased_next = values.erase_after(values.begin());
    CASTLE_SAMPLE_CHECK(*erased_next == 3U);
    CASTLE_SAMPLE_CHECK(values.size() == 3U);

    CASTLE_SAMPLE_CHECK(values.remove(3U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.find(3U) == values.end());
    CASTLE_SAMPLE_CHECK(values.size() == 2U);

    values.reverse();
    CASTLE_SAMPLE_CHECK(values.front() == 2U);

    CASTLE_SAMPLE_CHECK(values.pop_front() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.front() == 0U);

    values.clear();
    CASTLE_SAMPLE_CHECK(values.empty());
    CASTLE_SAMPLE_CHECK(values.begin() == values.end());
    CASTLE_SAMPLE_CHECK(values.pop_front() == castle::status::empty);
    CASTLE_SAMPLE_CHECK(values.erase_after(values.end()) == values.end());

    castle::container::forward_list<uint32_t, 4U> listed{7U, 8U, 9U};
    auto listed_it = listed.begin();
    CASTLE_SAMPLE_CHECK(*listed_it == 7U);
    ++listed_it;
    CASTLE_SAMPLE_CHECK(*listed_it == 8U);
    return 0;
}
