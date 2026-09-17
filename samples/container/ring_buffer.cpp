#include "sample_support.hpp"

#include "castle/container/ring_buffer.hpp"

int main()
{
    castle::container::ring_buffer<uint16_t, 3U> buffer{1U, 2U};
    CASTLE_SAMPLE_CHECK(buffer.size() == 2U);
    CASTLE_SAMPLE_CHECK(buffer.front() == 1U);
    CASTLE_SAMPLE_CHECK(buffer.back() == 2U);

    CASTLE_SAMPLE_CHECK(buffer.push(3U));
    CASTLE_SAMPLE_CHECK(buffer.full());
    CASTLE_SAMPLE_CHECK(!buffer.push(4U));

    uint16_t peeked = 0U;
    CASTLE_SAMPLE_CHECK(buffer.peek(1U, peeked));
    CASTLE_SAMPLE_CHECK(peeked == 2U);
    CASTLE_SAMPLE_CHECK(!buffer.peek(3U, peeked));

    CASTLE_SAMPLE_CHECK(buffer.force_push(4U));
    CASTLE_SAMPLE_CHECK(buffer.front() == 2U);
    CASTLE_SAMPLE_CHECK(buffer.back() == 4U);

    uint16_t popped = 0U;
    CASTLE_SAMPLE_CHECK(buffer.pop(popped));
    CASTLE_SAMPLE_CHECK(popped == 2U);
    CASTLE_SAMPLE_CHECK(buffer.pop());
    CASTLE_SAMPLE_CHECK(buffer.front() == 4U);

    uint16_t src[3U] = {5U, 6U, 7U};
    CASTLE_SAMPLE_CHECK(buffer.push_bulk(src, 3U) == 2U);
    CASTLE_SAMPLE_CHECK(buffer.full());

    uint16_t dst[3U] = {0U, 0U, 0U};
    CASTLE_SAMPLE_CHECK(buffer.pop_bulk(dst, 3U) == 3U);
    CASTLE_SAMPLE_CHECK(dst[0U] == 4U);
    CASTLE_SAMPLE_CHECK(dst[1U] == 5U);
    CASTLE_SAMPLE_CHECK(dst[2U] == 6U);
    CASTLE_SAMPLE_CHECK(buffer.empty());
    CASTLE_SAMPLE_CHECK(!buffer.pop());

    buffer.push(9U);
    buffer.clear();
    CASTLE_SAMPLE_CHECK(buffer.empty());

    return 0;
}
