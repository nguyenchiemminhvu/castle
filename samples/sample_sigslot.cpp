#include "sample_support.h"

#include "castle/utility/move.h"
#include "castle/events/sigslot.h"

// Scenario: fixed-capacity signal/slot notification from an ISR-safe producer to application logic.
#include <stdint.h>

int main()
{
    castle::sigslot::signal<3, void(uint32_t), 32U> signal;
    uint32_t total = 0U;
    auto token = signal.connect([&total](uint32_t value) { total += value; });
    signal.emit(6U);
    CASTLE_SAMPLE_CHECK(total == 6U);
    token.disconnect();
    return 0;
}
