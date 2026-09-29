#include <assert.h>

#include "castle_ext/io/gpio/types.hpp"

int main()
{
    using namespace castle::io::gpio;
    const pin_config config(direction::output, level::high, pull::up, drive::open_drain, true);
    assert(config.is_output());
    assert(!config.is_input());
    assert(is_high(config.initial));
    assert(!is_low(config.initial));
    assert(to_bool(to_level(true)));
    assert(invert(level::high) == level::low);
    assert(invert(level::low) == level::high);
    const mask_type mask = 0xA5U;
    assert(mask == 0xA5U);
    return 0;
}
