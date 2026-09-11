/**
 * Castle 2.0 sample: <castle/memory/destroy.h>
 *
 * Scenario: explicitly end an object's lifetime in a caller-owned static buffer.
 * The raw storage is never allocated from the heap and is reused deterministically.
 */
#include "sample_support.h"
#include "castle/memory/destroy.h"
#include "castle/memory/static_storage.h"
#include "castle/memory/construct.h"

#include <stdint.h>

struct Counter
{
    explicit Counter(uint32_t initial) : value(initial) {}
    ~Counter() noexcept { value = 0U; }
    uint32_t value;
};

int main()
{
    castle::memory::static_storage<Counter, 1U> storage;
    Counter* counter = castle::memory::construct_at<Counter>(storage.address(0U), 7U);
    CASTLE_SAMPLE_CHECK(counter->value == 7U);

    castle::memory::destroy_at(counter);
    return 0;
}
