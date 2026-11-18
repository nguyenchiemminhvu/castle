#include "sample_support.hpp"

#include "castle/utility/tuple.hpp"

#include <stdint.h>

using packet_type = castle::tuple<uint16_t, uint32_t>;
static_assert(castle::tuple_size<packet_type>::value == 2U, "tuple_size");
static_assert(castle::tuple_size_v<packet_type> == 2U, "tuple_size_v");
static_assert(castle::meta::is_same<castle::tuple_element_t<0U, packet_type>, uint16_t>::value, "tuple_element_t");

int main()
{
    packet_type packet(10U, 200U);
    CASTLE_SAMPLE_CHECK(packet.size() == 2U);
    CASTLE_SAMPLE_CHECK(castle::get<0>(packet) == 10U);
    CASTLE_SAMPLE_CHECK(castle::get<1>(packet) == 200U);

    auto made = castle::make_tuple(uint16_t(3U), uint32_t(4U));
    CASTLE_SAMPLE_CHECK(castle::get<uint16_t>(made) == 3U);
    CASTLE_SAMPLE_CHECK(castle::get<uint32_t>(made) == 4U);
    CASTLE_SAMPLE_CHECK(castle::get<0>(castle::move(made)) == 3U);

    uint16_t id = 0U;
    uint32_t value = 0U;
    auto refs = castle::tie(id, value);
    castle::get<0>(refs) = 5U;
    castle::get<1>(refs) = 9U;
    CASTLE_SAMPLE_CHECK(id == 5U && value == 9U);

    auto forwarded = castle::forward_as_tuple(id, value);
    castle::get<0>(forwarded) = 11U;
    CASTLE_SAMPLE_CHECK(id == 11U);

    castle::ignore = 42;
    return 0;
}
