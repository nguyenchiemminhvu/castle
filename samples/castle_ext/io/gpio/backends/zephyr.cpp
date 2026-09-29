#include <assert.h>

#include "castle_ext/io/gpio/backends/zephyr.hpp"

int main()
{
#if defined(CASTLE_EXT_ENABLE_GPIO_ZEPHYR)
    // A real binding needs a board-specific gpio_dt_spec from the devicetree.
    // See docs/castle_ext/io/gpio/backends/zephyr.md for the integration pattern.
    assert(true);
#else
    // Host builds can include this header without pulling in the Zephyr SDK.
    assert(true);
#endif
    return 0;
}
