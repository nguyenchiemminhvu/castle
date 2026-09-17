# Type Ranges

## Overview
This header provides Castle's constexpr numeric range metadata for built-in integral and floating-point types without depending on `std::numeric_limits`.

## Header
`#include "castle/core/type_ranges.hpp"`

## Dependencies
- [compiler.md](compiler.md)
- [types.md](types.md)

## Public API
| API | Description |
|---|---|
| `castle::numeric_limits<T>` | Primary template mirroring the subset of `std::numeric_limits` Castle needs. |
| `castle::numeric_limits<T>::is_specialized` | Reports whether Castle provides a specialization for `T`. |
| `castle::numeric_limits<T>::is_signed` | Reports whether `T` is signed. |
| `castle::numeric_limits<T>::is_integer` | Reports whether `T` is an integral type. |
| `castle::numeric_limits<T>::is_exact` | Reports whether `T` has exact integer-style representation. |
| `castle::numeric_limits<T>::min()`, `max()`, `lowest()` | Return the compile-time lower/upper bounds Castle uses for supported built-in types. |

## Usage Example
```cpp
#include "castle/core/type_ranges.hpp"

static_assert(castle::numeric_limits<unsigned int>::max() >= 65535U, "range available");
```

See also: `samples/sample_type_ranges.cpp`.

## Constraints & Notes
- Covers the built-in scalar types Castle uses; unsupported types have only the primary template declaration.
- Falls back to compiler width macros when some freestanding environments omit standard `LONG_*` macros.
- Exposes only a small `numeric_limits` subset needed by Castle internals.
