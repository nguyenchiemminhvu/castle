#include "sample_support.hpp"

#include "castle/container/heap.hpp"

#include <stdint.h>

int main()
{
    castle::container::min_heap<uint32_t, 6U> heap{40U, 10U, 30U};
    CASTLE_SAMPLE_CHECK(heap.capacity() == 6U);
    CASTLE_SAMPLE_CHECK(heap.size() == 3U);
    CASTLE_SAMPLE_CHECK(heap.available() == 3U);
    CASTLE_SAMPLE_CHECK(heap.top() == 10U);

    uint32_t moved_in = 5U;
    CASTLE_SAMPLE_CHECK(heap.push(20U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(heap.push(castle::move(moved_in)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(heap.emplace(12U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(heap.full());
    CASTLE_SAMPLE_CHECK(heap.push(1U) == castle::status::full);
    CASTLE_SAMPLE_CHECK(heap.top() == 5U);

    CASTLE_SAMPLE_CHECK(heap.begin() != nullptr);
    CASTLE_SAMPLE_CHECK(heap.end() - heap.begin() == static_cast<castle::difference_type>(heap.size()));

    uint32_t popped = 0U;
    CASTLE_SAMPLE_CHECK(heap.pop(popped) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(popped == 5U);
    CASTLE_SAMPLE_CHECK(heap.top() == 10U);

    uint32_t replacement = 17U;
    CASTLE_SAMPLE_CHECK(heap.replace_top(replacement) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(heap.top() == 12U);

    const uint32_t replacement_const = 4U;
    CASTLE_SAMPLE_CHECK(heap.replace_top(replacement_const) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(heap.top() == 4U);

    CASTLE_SAMPLE_CHECK(heap.remove_at(1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(heap.size() == 4U);
    CASTLE_SAMPLE_CHECK(heap.remove_at(9U) == castle::status::out_of_range);

    CASTLE_SAMPLE_CHECK(heap.pop() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!heap.empty());

    castle::container::max_heap<uint32_t, 4U> max_values;
    CASTLE_SAMPLE_CHECK(max_values.push(2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(max_values.push(7U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(max_values.push(5U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(max_values.top() == 7U);

    heap.clear();
    CASTLE_SAMPLE_CHECK(heap.empty());
    CASTLE_SAMPLE_CHECK(heap.begin() == nullptr);
    CASTLE_SAMPLE_CHECK(heap.end() == nullptr);
    CASTLE_SAMPLE_CHECK(heap.pop() == castle::status::empty);
    CASTLE_SAMPLE_CHECK(heap.replace_top(replacement) == castle::status::empty);
    return 0;
}
