#include "sample_support.hpp"

#include "castle/container/mirrored_ring_buffer.hpp"

// Scenario: a DMA-friendly SPSC queue whose logical payload remains contiguous
// even after the ring wraps around the physical end of the first storage half.
int main()
{
    castle::container::mirrored_ring_buffer<uint16_t, 4U> buffer;

    CASTLE_SAMPLE_CHECK(buffer.empty());
    CASTLE_SAMPLE_CHECK(buffer.push(10U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.push(20U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.push(30U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.pop() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.push(40U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.push(50U) == castle::status::ok);

    CASTLE_SAMPLE_CHECK(buffer.size() == 4U);
    CASTLE_SAMPLE_CHECK(buffer.begin()[0U] == 20U);
    CASTLE_SAMPLE_CHECK(buffer.begin()[1U] == 30U);
    CASTLE_SAMPLE_CHECK(buffer.begin()[2U] == 40U);
    CASTLE_SAMPLE_CHECK(buffer.begin()[3U] == 50U);

    // Starting from slot zero, the second physical half mirrors the first.
    // The mirrored storage is internal; the important public property is that
    // the wrapped logical sequence remains one contiguous iterator range.
    auto first = buffer.begin();
    CASTLE_SAMPLE_CHECK(first[0U] == 20U);
    CASTLE_SAMPLE_CHECK(first[3U] == 50U);
    for (auto it = buffer.begin(); it != buffer.end(); ++it)
    {
        CASTLE_SAMPLE_CHECK(it >= buffer.begin());
        CASTLE_SAMPLE_CHECK(it < buffer.end());
    }

    // DMA/IO-style access: reserve a contiguous region, hand its pointer and
    // size to a peripheral or driver, then publish it in a single commit.
    // write_reserve() may return a shorter view than requested once the
    // reservation would otherwise cross the wrap point of the *next* commit.
    buffer.clear();
    CASTLE_SAMPLE_CHECK(buffer.push(1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.pop() == castle::status::ok);

    castle::container::array_view<uint16_t> write = buffer.write_reserve(3U);
    CASTLE_SAMPLE_CHECK(write.size() == 3U);
    for (castle::size_type i = 0U; i < write.size(); ++i)
    {
        write[i] = static_cast<uint16_t>(100U + i);
    }
    CASTLE_SAMPLE_CHECK(buffer.write_commit(write) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.size() == 3U);

    castle::container::array_view<uint16_t> read = buffer.read_reserve();
    CASTLE_SAMPLE_CHECK(read.size() == 3U);
    CASTLE_SAMPLE_CHECK(read[0U] == 100U);
    CASTLE_SAMPLE_CHECK(read[2U] == 102U);
    CASTLE_SAMPLE_CHECK(buffer.read_commit(read) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffer.empty());

    // push_bulk()/pop_bulk() are the non-DMA equivalent for plain memory
    // transfers and internally reuse the same reservation/commit machinery.
    CASTLE_CONST uint16_t src[] = {7U, 8U, 9U, 11U};
    CASTLE_SAMPLE_CHECK(buffer.push_bulk(src, 4U) == 4U);
    CASTLE_SAMPLE_CHECK(buffer.full());

    uint16_t dst[4] = {};
    CASTLE_SAMPLE_CHECK(buffer.pop_bulk(dst, 4U) == 4U);
    CASTLE_SAMPLE_CHECK(dst[0U] == 7U);
    CASTLE_SAMPLE_CHECK(dst[3U] == 11U);
    CASTLE_SAMPLE_CHECK(buffer.empty());

    return 0;
}
