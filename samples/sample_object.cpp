#include "sample_support.h"

#include "castle/memory/object.h"

// Scenario: recovering a typed object view from a register-aligned address.
#include <stdint.h>

int main()
{
    uint32_t raw = 5U;
    auto* p = castle::memory::object_from_address<uint32_t>(&raw);
    CASTLE_SAMPLE_CHECK(*p == 5U);
    return 0;
}
