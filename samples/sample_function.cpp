#include "sample_support.h"

#include "castle/callbacks/function.h"

// Scenario: registering a deterministic scheduler callback with no streams or operating-system services.
#include <stdint.h>

namespace
{
uint32_t g_ticks = 0U;
void on_tick(uint32_t ticks) noexcept
{
    g_ticks += ticks;
}
} // namespace

int main()
{
    castle::callbacks::function<void(uint32_t)> callback(on_tick);
    callback(10U);
    CASTLE_SAMPLE_CHECK(g_ticks == 10U);
    return 0;
}
