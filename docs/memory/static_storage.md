# Static Storage

## Overview
Names fixed-capacity raw storage for `T` objects. It exists as a clearer alias for `raw_storage<T, N>` when the intent is "preallocated slots, explicit construction later."

## Header
`#include "castle/memory/static_storage.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Storage](storage.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T, size_t N> using castle::memory::static_storage = raw_storage<T, N>` | Alias for `N` aligned raw slots sized for `T`. Inherits `capacity`, `address(index)`, and `bytes()` from `raw_storage`. |

## Usage Example
```cpp
// See: samples/sample_static_storage.cpp
castle::memory::static_storage<uint32_t, 4U> storage;
void* slot = storage.address(0U);
```

## Constraints & Notes
- No object is constructed automatically.
- Access rules and zero-capacity behavior are exactly the same as `raw_storage`.
- Callers must explicitly manage lifetime for anything placed into the slots.
