#include "sample_support.h"

#include "castle/utility/move.h"
#include "castle/callbacks/inplace_function.h"

// Scenario: binding a small ISR-safe callable entirely inside inline storage.
#include <stdint.h>

int main()
{
    uint32_t accumulator = 0U;
    castle::callbacks::inplace_function<void(uint32_t), 32U> callback =
        [&accumulator](uint32_t value) { accumulator += value; };

    callback(7U);
    callback(5U);
    CASTLE_SAMPLE_CHECK(accumulator == 12U);
    return 0;
}
