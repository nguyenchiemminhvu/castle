#include "sample_support.h"

#include "castle/container/heap.h"

// Scenario: bounded deadline queue where the next smallest timeout must be at the root.
int main()
{
    castle::container::basic_heap<uint32_t, 8U, castle::less<uint32_t>> heap;
    CASTLE_SAMPLE_CHECK(heap.push(40U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(heap.push(10U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(heap.push(25U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(heap.top() == 10U);
    CASTLE_SAMPLE_CHECK(heap.pop() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(heap.top() == 25U);
    return 0;
}
