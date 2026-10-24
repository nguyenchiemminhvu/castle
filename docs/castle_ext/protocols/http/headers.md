# Bounded HTTP Headers

## Overview
`basic_headers` stores arbitrary HTTP fields in a compile-time-sized array. It owns bounded copies of names and values and supports HTTP/2 pseudo-header names beginning with `:`.

## Header
`#include "castle_ext/protocols/http/headers.hpp"`

## Dependencies
- [HTTP shared types](types.md)
- [CASTLE compiler annotations](../../../../include/castle/core/compiler.hpp)
- [CASTLE core types](../../../../include/castle/core/types.hpp)
- [CASTLE status](../../../../include/castle/error/status.hpp)

## Public API
- `basic_headers<MaxHeaders, MaxNameSize, MaxValueSize>`: fixed-capacity owning collection.
- `add(name, value)`: appends a field; returns `full`, `out_of_range`, or `invalid_argument` for capacity/validation failures.
- `set(name, value)`: replaces the first case-insensitive matching field or appends a new one.
- `get(name)`, `contains(name)`: case-insensitive lookup; lookup is O(MaxHeaders).
- `remove(name)`: removes the first matching field and compacts remaining entries; O(MaxHeaders).
- `operator[](index)`: returns a non-owning `header_view`; an out-of-range index returns an empty view.
- `clear`, `size`, `empty`, `full`: capacity and state helpers.

## Usage Example
```cpp
#include "castle_ext/protocols/http/headers.hpp"

int main()
{
    castle::protocols::http::basic_headers<8U, 32U, 96U> headers;
    if (headers.add("accept", "application/json") != castle::status::ok) { return 1; }
    if (headers.set("user-agent", "castle-device/1") != castle::status::ok) { return 2; }
    return headers.contains("ACCEPT") ? 0 : 3;
}
```

## Constraints & Notes
Storage is inline: approximately `MaxHeaders * (MaxNameSize + MaxValueSize + 2)` bytes plus bookkeeping. Names must be HTTP token characters, with a single leading colon accepted for pseudo-headers. Values reject CR, LF, NUL, DEL, and controls other than horizontal tab. Duplicate fields are allowed by `add`; `get` returns the first. No heap allocation, exceptions, RTTI, or STL dependency.
