#include "sample_support.hpp"

#include "castle/container/containers.hpp"

int main()
{
    castle::container::array<uint8_t, 3U> bytes{1U, 2U, 3U};
    CASTLE_SAMPLE_CHECK(bytes.front() == 1U);
    CASTLE_SAMPLE_CHECK(bytes.back() == 3U);

    castle::container::map<uint8_t, uint16_t, 2U> ordered;
    CASTLE_SAMPLE_CHECK(ordered.insert(2U, 20U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(ordered.insert(1U, 10U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(ordered.begin()->first == 1U);

    castle::container::hash_set<uint8_t, 2U> enabled;
    CASTLE_SAMPLE_CHECK(enabled.insert(7U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(enabled.contains(7U));

    castle::container::ring_buffer<uint8_t, 2U> fifo;
    CASTLE_SAMPLE_CHECK(fifo.push(9U));
    CASTLE_SAMPLE_CHECK(fifo.front() == 9U);

    castle::container::string_view label("OK");
    CASTLE_SAMPLE_CHECK(label.size() == 2U);
    CASTLE_SAMPLE_CHECK(label.back() == 'K');

    return 0;
}
