#include "sample_support.h"

#include "castle/memory/construct.h"
#include "castle/memory/alignment.h"

// Scenario: constructing a peripheral-state object inside preallocated storage.
#include <stdint.h>
struct Counter
{
    explicit Counter(uint32_t v) : value(v) {} uint32_t value;
};

int main()
{
    castle::memory::aligned_storage_as_t<sizeof(Counter), Counter> storage;
    auto* p = castle::memory::construct_at<Counter>(storage.get_address<Counter>(), 42U);
    CASTLE_SAMPLE_CHECK(p->value == 42U);
    p->~Counter();
    return 0;
}
