#include "sample_support.h"

#include "castle/memory/static_storage.h"

// Scenario: owning a fixed pool of uninitialised slots for deterministic object construction.
#include <stdint.h>
int main()
{
    castle::memory::static_storage<uint32_t, 4U> storage;
    auto* first = storage.address(0U);
    auto* last = storage.address(3U);
    CASTLE_SAMPLE_CHECK(first != last);
    return 0;
}
