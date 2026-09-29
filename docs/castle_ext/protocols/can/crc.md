# CAN CRC

## Overview

Calculates the bit-oriented frame check sequence for Classical CAN (CRC-15) and ISO CAN FD (CRC-17 or CRC-21). It includes identifier/control fields, payload bits, and CAN FD dynamic stuffing/stuff-count input.

## Header

`#include "castle_ext/protocols/can/crc.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`castle_ext/protocols/can/frame.hpp`](frame.md)

## Public API

| API | Description |
|---|---|
| `crc_result` | Holds a checksum `value` and checksum `width` in bits. |
| `calculate_crc(frame, output)` | Calculates the FCS into `output`; returns `castle::status::ok` on success. |
| `CAN_CLASSICAL_CRC_POLYNOMIAL` | Classical CAN CRC-15 polynomial `0x4599`. |
| `CAN_FD_CRC17_POLYNOMIAL` | CAN FD CRC-17 polynomial `0x1685B`. |
| `CAN_FD_CRC21_POLYNOMIAL` | CAN FD CRC-21 polynomial `0x102899`. |

## Usage Example

```cpp
#include "castle_ext/protocols/can/crc.hpp"
#include <assert.h>

int main()
{
    using namespace castle::protocols::can;
    const uint8_t bytes[1] = {0xA5U};
    frame value{};
    assert(castle::succeeded(make_data_frame(
        0x321U, identifier_format::standard, frame_format::classical,
        bytes, 1U, value)));
    crc_result result{};
    assert(castle::succeeded(calculate_crc(value, result)));
    assert(result.width == CAN_CLASSICAL_CRC_WIDTH);
    return 0;
}
```

See [`samples/castle_ext/protocols/can/crc.cpp`](../../../../samples/castle_ext/protocols/can/crc.cpp).

## Constraints & Notes

- `calculate_crc()` accepts valid data and remote frames. Error/overload signals do not carry an application FCS and are rejected.
- Classical CAN CRC input excludes dynamically inserted stuff bits. ISO CAN FD includes dynamic stuff bits and the Gray-coded stuff count plus parity, and starts from the protocol-defined leading-one state.
- The existing Castle CRC-8/16/32/64 helpers are byte-stream algorithms with incompatible widths/parameters, so reusing them directly would compute the wrong CAN FCS. This header uses a bounded bitwise calculation instead.
- O(1) auxiliary memory; runtime is linear in header plus payload bits. It does not emit bit stuffing/CRC delimiter/ACK/EOF on a physical bus.
