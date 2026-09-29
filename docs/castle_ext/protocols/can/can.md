# CAN Protocol Umbrella

## Overview

Includes Castle's CAN frame model, Classical CAN/CAN FD checksum calculation, and callback-based sender/receiver node facades through one public header.

## Header

`#include "castle_ext/protocols/can/can.hpp"`

## Dependencies

- [`frame.hpp`](frame.md)
- [`crc.hpp`](crc.md)
- [`node.hpp`](node.md)

## Public API

The umbrella exposes the complete public API from the component headers: frame types and factories, `calculate_crc()`, `sender_node`, `receiver_node`, and their callback types.

## Usage Example

```cpp
#include "castle_ext/protocols/can/can.hpp"
#include <assert.h>

int main()
{
    using namespace castle::protocols::can;
    frame value{};
    assert(castle::succeeded(make_data_frame(0x45U, identifier_format::standard,
        frame_format::classical, nullptr, 0U, value)));
    crc_result check{};
    assert(castle::succeeded(calculate_crc(value, check)));
    assert(check.width == CAN_CLASSICAL_CRC_WIDTH);
    return 0;
}
```

See [`samples/castle_ext/protocols/can/can.cpp`](../../../../samples/castle_ext/protocols/can/can.cpp).

## Constraints & Notes

All constraints of the included headers apply. The component is hardware-independent: a target-specific controller adapter is needed for actual network transmission/reception and electrical-layer behavior.
