#include "sample_support.hpp"

#include "castle/container/vector.hpp"

#include <stdint.h>

int main()
{
    castle::container::vector<uint16_t, 4U> values;
    CASTLE_SAMPLE_CHECK(values.empty());
    CASTLE_SAMPLE_CHECK(values.capacity() == 4U);
    CASTLE_SAMPLE_CHECK(values.static_capacity == 4U);

    uint16_t first = 10U;
    CASTLE_SAMPLE_CHECK(values.push_back(first) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.emplace_back(20U) == castle::status::ok);

    uint16_t third = 30U;
    CASTLE_SAMPLE_CHECK(values.push_back(castle::move(third)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.size() == 3U);
    CASTLE_SAMPLE_CHECK(values.front() == 10U);
    CASTLE_SAMPLE_CHECK(values.back() == 30U);

    values[1U] = 21U;
    CASTLE_SAMPLE_CHECK(values[1U] == 21U);
    CASTLE_SAMPLE_CHECK(values.data()[2U] == 30U);

    castle::size_type forward_sum = 0U;
    for (auto it = values.begin(); it != values.end(); ++it)
    {
        forward_sum += *it;
    }
    CASTLE_SAMPLE_CHECK(forward_sum == 61U);

    const castle::container::vector<uint16_t, 4U>& cvalues = values;
    CASTLE_SAMPLE_CHECK(cvalues.cbegin()[0U] == 10U);
    CASTLE_SAMPLE_CHECK(cvalues.cend() - cvalues.cbegin() == static_cast<castle::difference_type>(values.size()));

    auto reverse = values.rbegin();
    CASTLE_SAMPLE_CHECK(*reverse == 30U);
    ++reverse;
    CASTLE_SAMPLE_CHECK(*reverse == 21U);
    CASTLE_SAMPLE_CHECK(values.crbegin() != values.crend());

    castle::container::vector<uint16_t, 4U> copy(values);
    CASTLE_SAMPLE_CHECK(copy.size() == values.size());
    CASTLE_SAMPLE_CHECK(copy[0U] == 10U);

    castle::container::vector<uint16_t, 4U> assigned;
    assigned = values;
    CASTLE_SAMPLE_CHECK(assigned.back() == 30U);

    castle::container::vector<uint16_t, 4U> moved(castle::move(copy));
    CASTLE_SAMPLE_CHECK(moved.size() == 3U);
    CASTLE_SAMPLE_CHECK(copy.empty());

    castle::container::vector<uint16_t, 4U> move_assigned;
    move_assigned = castle::move(assigned);
    CASTLE_SAMPLE_CHECK(move_assigned.size() == 3U);
    CASTLE_SAMPLE_CHECK(assigned.empty());

    castle::container::vector<uint16_t, 4U> listed{1U, 2U, 3U};
    CASTLE_SAMPLE_CHECK(listed.size() == 3U);
    listed = {4U, 5U};
    CASTLE_SAMPLE_CHECK(listed.size() == 2U);
    CASTLE_SAMPLE_CHECK(listed.front() == 4U);

    CASTLE_SAMPLE_CHECK(values.push_back(40U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.full());
    CASTLE_SAMPLE_CHECK(values.push_back(50U) == castle::status::full);
    CASTLE_SAMPLE_CHECK(values.pop_back() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.back() == 30U);

    values.clear();
    CASTLE_SAMPLE_CHECK(values.empty());
    CASTLE_SAMPLE_CHECK(values.begin() == values.end());
    CASTLE_SAMPLE_CHECK(values.pop_back() == castle::status::empty);
    return 0;
}
