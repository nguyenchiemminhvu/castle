# LIN Frame

## Overview

Defines validated LIN frame identifiers, parity-protected identifier (PID) bytes, logical headers, response frames, and protocol constants. The frame is fixed-capacity and stores up to eight data bytes inline.

## Header

`#include "castle_ext/protocols/lin/frame.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)

## Public API

| API | Description |
|---|---|
| `is_valid_identifier(id)` | Returns true for usable LIN 2.x identifiers 0–61. IDs 62–63 are reserved. O(1). |
| `calculate_protected_identifier(id)` | Calculates PID parity; returns `LIN_INVALID_PROTECTED_IDENTIFIER` for reserved IDs. O(1). |
| `is_valid_protected_identifier(pid)` | Checks identifier range and both parity bits. O(1). |
| `header` | Holds `identifier` and `protected_identifier`; `valid()` validates both. |
| `frame` | Holds identifier, PID, length, inline payload, checksum and checksum mode; `valid()` checks structure, not checksum value. |
| `make_header(id, output)` | Builds a header, leaving `output` unchanged on failure. |
| `decode_header(pid, output)` | Decodes a PID byte to a header, leaving `output` unchanged on failure. |
| `checksum_type` | Selects classic or enhanced checksum. |
| `response_owner` | Identifies master- or slave-published response ownership for schedule slots. |

## Usage Example

```cpp
#include "castle_ext/protocols/lin/frame.hpp"
#include <assert.h>

int main()
{
    using namespace castle::protocols::lin;
    header request{};
    assert(castle::succeeded(make_header(0x12U, request)));
    assert(request.valid());
    header decoded{};
    assert(castle::succeeded(decode_header(request.protected_identifier, decoded)));
    assert(decoded.identifier == request.identifier);
    return 0;
}
```

See [`samples/castle_ext/protocols/lin/frame.cpp`](../../../../samples/castle_ext/protocols/lin/frame.cpp).

## Constraints & Notes

No allocation or platform I/O occurs. LIN response payloads contain 1–8 bytes; diagnostic identifiers 0x3C and 0x3D require eight bytes and classic checksum. Physical break/sync timing and UART encoding belong to the driver.
