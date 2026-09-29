# LIN Checksum

## Overview

Calculates LIN classic and enhanced response checksums and constructs complete response frames. Enhanced checksum includes the parity-protected identifier; classic checksum includes only payload bytes. Diagnostic identifiers 0x3C and 0x3D always use classic checksum.

## Header

`#include "castle_ext/protocols/lin/checksum.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/container/array_view.hpp`](../../../../include/castle/container/array_view.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`frame.hpp`](frame.md)

## Public API

| API | Description |
|---|---|
| `effective_checksum_type(id, requested)` | Returns classic for diagnostic IDs, otherwise preserves the requested model. O(1). |
| `calculate_checksum(id, data, length, type, output)` | Calculates the checksum; returns `invalid_argument` for invalid ID, length, pointer, or mode. O(length). |
| `calculate_checksum(frame, output)` | Calculates for a structurally valid frame. O(length). |
| `calculate_checksum(id, array_view, type, output)` | Overload that accepts a non-owning Castle byte view. O(length). |
| `validate_checksum(frame)` | Returns true only when both structure and checksum match. O(length). |
| `make_frame(id, data, length, type, output)` | Copies payload inline and calculates a checksum; output changes only on success. O(length). |
| `make_frame(id, array_view, type, output)` | Same factory for a non-owning Castle byte view. O(length). |

## Usage Example

```cpp
#include "castle_ext/protocols/lin/checksum.hpp"
#include <assert.h>

int main()
{
    using namespace castle::protocols::lin;
    const uint8_t payload[2U] = {0x12U, 0x34U};
    frame value{};
    assert(castle::succeeded(make_frame(0x12U, payload, 2U,
        checksum_type::enhanced, value)));
    assert(validate_checksum(value));
    return 0;
}
```

See [`samples/castle_ext/protocols/lin/checksum.cpp`](../../../../samples/castle_ext/protocols/lin/checksum.cpp).

## Constraints & Notes

The algorithm uses an eight-bit end-around-carry sum and its one's complement. Payload length is 1–8 bytes; diagnostic frame IDs require exactly eight bytes. All storage is fixed-capacity and local.
