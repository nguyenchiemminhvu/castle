#include "sample_support.hpp"

#include "castle/cache/lifo.hpp"

// Parameter cache where the first entries loaded at boot form a stable base set; later, transient
// lookups replace each other on top of it like a bounded stack.
int main()
{
    castle::cache::lifo<uint16_t, int32_t, 3U> params; // parameter ID -> value

    params.put(1U, 115200); // baud rate, loaded at boot
    params.put(2U, 8);      // data bits, loaded at boot
    params.put(50U, -3);    // transient diagnostic value
    CASTLE_SAMPLE_CHECK(params.full());

    uint16_t dropped = 0U;
    params.put(51U, 7, [&dropped](const uint16_t& id, int32_t&) { dropped = id; });
    CASTLE_SAMPLE_CHECK(dropped == 50U); // newest entry leaves first

    params.put(52U, 9, [&dropped](const uint16_t& id, int32_t&) { dropped = id; });
    CASTLE_SAMPLE_CHECK(dropped == 51U);

    // The boot-time base set survived every rotation.
    CASTLE_SAMPLE_CHECK(*params.get(1U) == 115200);
    CASTLE_SAMPLE_CHECK(*params.get(2U) == 8);
    CASTLE_SAMPLE_CHECK(params.size() == 3U);

    return 0;
}
