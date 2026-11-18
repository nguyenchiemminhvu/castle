#include "sample_support.hpp"

#include "castle/memory/alignment.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"

#include <stdint.h>

struct Counter
{
    Counter(uint32_t initial, uint32_t step_value) : value(initial), step(step_value) {}
    ~Counter() noexcept
    {
        value = 0U;
        step = 0U;
    }

    uint32_t value;
    uint32_t step;
};

int main()
{
    castle::memory::aligned_storage_as_t<sizeof(Counter), Counter> storage{};
    Counter* counter = castle::memory::construct_at<Counter>(storage.get_address<Counter>(), 42U, 3U);

    CASTLE_SAMPLE_CHECK(counter == storage.get_address<Counter>());
    CASTLE_SAMPLE_CHECK(counter->value == 42U);
    CASTLE_SAMPLE_CHECK(counter->step == 3U);

    castle::memory::destroy_at(counter);
    return 0;
}
