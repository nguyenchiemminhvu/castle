#include "sample_support.hpp"

#include "castle/memory/object.hpp"
#include "castle/memory/alignment.hpp"

// Scenario: accessing typed objects in manually controlled embedded storage.
#include <stdint.h>
int main()
{
    castle::memory::aligned_storage_as_t<sizeof(uint32_t), uint32_t> storage;
    auto* p = castle::memory::object_from_address<uint32_t>(storage.get_address<uint32_t>());
    *p = 0x55U;
    CASTLE_SAMPLE_CHECK(*p == 0x55U);
    return 0;
}
