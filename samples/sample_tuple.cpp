#include "sample_support.h"

#include "castle/utility/tuple.h"

// Scenario: packing heterogeneous telemetry metadata while preserving static type information.
#include <stdint.h>
int main()
{
    auto packet = castle::make_tuple(uint16_t(10U), uint32_t(200U));
    CASTLE_SAMPLE_CHECK(castle::get<0>(packet) == 10U);
    CASTLE_SAMPLE_CHECK(castle::get<uint32_t>(packet) == 200U);
    uint16_t id = 0U;
    uint32_t value = 0U;
    auto refs = castle::tie(id, value);
    castle::get<0>(refs) = 5U;
    castle::get<1>(refs) = 9U;
    CASTLE_SAMPLE_CHECK(id == 5U && value == 9U);
    static_assert(castle::tuple_size_v<decltype(packet)> == 2U, "tuple size");
    return 0;
}
