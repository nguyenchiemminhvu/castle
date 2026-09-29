# LIN Protocol Umbrella

## Overview

Includes Castle's LIN frame/header model, classic and enhanced checksum utilities, fixed-capacity master schedule, and callback-based master/slave node facades.

## Header

`#include "castle_ext/protocols/lin/lin.hpp"`

## Dependencies

- [`frame.hpp`](frame.md)
- [`checksum.hpp`](checksum.md)
- [`schedule.hpp`](schedule.md)
- [`node.hpp`](node.md)

## Public API

The umbrella exposes the complete API from each component: PID/header construction and validation, `frame`, `make_frame()`, checksum calculation/validation, `schedule_table`, `master_node`, and `slave_node`.

## Usage Example

```cpp
#include "castle_ext/protocols/lin/lin.hpp"
#include <assert.h>

int main()
{
    using namespace castle::protocols::lin;
    const uint8_t bytes[1U] = {0xA5U};
    frame value{};
    assert(castle::succeeded(make_frame(0x12U, bytes, 1U,
        checksum_type::enhanced, value)));
    assert(validate_checksum(value));
    return 0;
}
```

See [`samples/castle_ext/protocols/lin/lin.cpp`](../../../../samples/castle_ext/protocols/lin/lin.cpp).

## Constraints & Notes

All constraints of the component headers apply. This extension is hardware-independent and does not replace a LIN transceiver, UART driver, schedule timer, or full LIN transport/diagnostic configuration stack.
