#include "sample_support.hpp"

#include "castle/container/stack.hpp"

int main()
{
    castle::container::stack_buffer<uint32_t, 3U> stack{1U, 2U};
    CASTLE_SAMPLE_CHECK(stack.size() == 2U);
    CASTLE_SAMPLE_CHECK(stack.top() == 2U);

    uint32_t peeked = 0U;
    CASTLE_SAMPLE_CHECK(stack.peek(0U, peeked));
    CASTLE_SAMPLE_CHECK(peeked == 1U);
    CASTLE_SAMPLE_CHECK(stack.peek(1U, peeked));
    CASTLE_SAMPLE_CHECK(peeked == 2U);

    CASTLE_SAMPLE_CHECK(stack.push(3U));
    CASTLE_SAMPLE_CHECK(stack.full());
    CASTLE_SAMPLE_CHECK(!stack.push(4U));
    CASTLE_SAMPLE_CHECK(stack.force_push(4U));
    CASTLE_SAMPLE_CHECK(stack.top() == 4U);
    CASTLE_SAMPLE_CHECK(stack.peek(2U, peeked));
    CASTLE_SAMPLE_CHECK(peeked == 4U);

    uint32_t popped = 0U;
    CASTLE_SAMPLE_CHECK(stack.pop(popped));
    CASTLE_SAMPLE_CHECK(popped == 4U);
    CASTLE_SAMPLE_CHECK(stack.pop());
    CASTLE_SAMPLE_CHECK(stack.top() == 1U);

    uint32_t src[2U] = {5U, 6U};
    CASTLE_SAMPLE_CHECK(stack.push_bulk(src, 2U) == 2U);
    CASTLE_SAMPLE_CHECK(stack.full());

    uint32_t dst[3U] = {0U, 0U, 0U};
    CASTLE_SAMPLE_CHECK(stack.pop_bulk(dst, 3U) == 3U);
    CASTLE_SAMPLE_CHECK(dst[0U] == 6U);
    CASTLE_SAMPLE_CHECK(dst[1U] == 5U);
    CASTLE_SAMPLE_CHECK(dst[2U] == 1U);
    CASTLE_SAMPLE_CHECK(stack.empty());
    CASTLE_SAMPLE_CHECK(!stack.pop());

    stack.push(9U);
    stack.clear();
    CASTLE_SAMPLE_CHECK(stack.empty());

    return 0;
}
