#include "sample_support.hpp"

#include "castle/container/mirrored_ring_buffer.hpp"

#include <stdint.h>

int main()
{
    castle::container::mirrored_ring_buffer<uint16_t, 4U> buffer;
    CASTLE_SAMPLE_CHECK(buffer.capacity() == 4U);
    CASTLE_SAMPLE_CHECK(buffer.max_size() == 4U);
    CASTLE_SAMPLE_CHECK(buffer.static_capacity == 4U);
    CASTLE_SAMPLE_CHECK(buffer.empty());
    CASTLE_SAMPLE_CHECK(buffer.available() == 4U);

    uint16_t first = 1U;
    CASTLE_SAMPLE_CHECK(buffer.push(first) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.push(uint16_t(2U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.push(uint16_t(3U)) == castle::status::ok);

    uint16_t popped = 0U;
    CASTLE_SAMPLE_CHECK(buffer.pop(popped) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(popped == 1U);

    CASTLE_SAMPLE_CHECK(buffer.push(uint16_t(4U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.push(uint16_t(5U)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.full());
    CASTLE_SAMPLE_CHECK(buffer.available() == 0U);
    CASTLE_SAMPLE_CHECK(buffer.push(uint16_t(6U)) == castle::status::full);

    CASTLE_SAMPLE_CHECK(buffer.front() == 2U);
    CASTLE_SAMPLE_CHECK(buffer.back() == 5U);
    CASTLE_SAMPLE_CHECK(buffer[1U] == 3U);
    CASTLE_SAMPLE_CHECK(buffer.data() == buffer.begin());

    const auto& cbuffer = buffer;
    CASTLE_SAMPLE_CHECK(cbuffer.cbegin()[0U] == 2U);
    CASTLE_SAMPLE_CHECK(cbuffer.cend() - cbuffer.cbegin() == static_cast<castle::difference_type>(buffer.size()));

    uint16_t peeked = 0U;
    CASTLE_SAMPLE_CHECK(buffer.peek(2U, peeked) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(peeked == 4U);
    CASTLE_SAMPLE_CHECK(buffer.peek(4U, peeked) == castle::status::out_of_range);

    CASTLE_SAMPLE_CHECK(buffer.pop() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.pop(popped) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(popped == 3U);
    CASTLE_SAMPLE_CHECK(buffer.size() == 2U);

    buffer.clear();
    CASTLE_SAMPLE_CHECK(buffer.empty());

    uint16_t scratch[1] = {0U};
    CASTLE_SAMPLE_CHECK(buffer.write_commit(castle::container::array_view<uint16_t>(scratch, 1U)) == castle::status::invalid_argument);

    auto optimal = buffer.write_reserve_optimal(3U);
    CASTLE_SAMPLE_CHECK(optimal.size() == 4U);

    auto write = buffer.write_reserve(3U);
    CASTLE_SAMPLE_CHECK(write.size() == 3U);
    for (castle::size_type i = 0U; i < write.size(); ++i)
    {
        write[i] = static_cast<uint16_t>(10U + i);
    }
    CASTLE_SAMPLE_CHECK(buffer.write_commit(write) == castle::status::ok);

    auto read = buffer.read_reserve();
    CASTLE_SAMPLE_CHECK(read.size() == 3U);
    CASTLE_SAMPLE_CHECK(read[0U] == 10U);
    CASTLE_SAMPLE_CHECK(read[2U] == 12U);
    CASTLE_SAMPLE_CHECK(buffer.read_commit(castle::container::array_view<uint16_t>(scratch, 1U)) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(buffer.read_commit(castle::container::array_view<uint16_t>(read.data(), 4U)) == castle::status::empty);
    CASTLE_SAMPLE_CHECK(buffer.read_commit(read) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.empty());

    const uint16_t src[4] = {7U, 8U, 9U, 11U};
    CASTLE_SAMPLE_CHECK(buffer.push_bulk(src, 4U) == 4U);
    CASTLE_SAMPLE_CHECK(buffer.full());

    uint16_t dst[4] = {};
    CASTLE_SAMPLE_CHECK(buffer.pop_bulk(dst, 4U) == 4U);
    CASTLE_SAMPLE_CHECK(dst[0U] == 7U);
    CASTLE_SAMPLE_CHECK(dst[3U] == 11U);
    CASTLE_SAMPLE_CHECK(buffer.empty());
    CASTLE_SAMPLE_CHECK(buffer.pop() == castle::status::empty);
    return 0;
}
