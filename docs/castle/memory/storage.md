# Storage

## Overview
Provides fixed-capacity raw storage indexed in `T`-sized slots. It exists so containers and applications can reserve deterministic bytes for later placement construction without using heap allocation.

## Header
`#include "castle/memory/storage.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T, size_t N> class castle::memory::raw_storage` | Raw storage for `N` contiguous slots aligned and sized for `T`. |
| `raw_storage<T, N>::value_type` | Alias for `T`. |
| `raw_storage<T, N>::capacity` | Compile-time slot count. |
| `void* address(size_t index)` / `void const* address(size_t index) const` | Returns the start address of slot `index`. O(1). |
| `uint8_t* bytes()` / `uint8_t const* bytes() const` | Returns the backing byte buffer. O(1). |
| `template <typename T> class castle::memory::raw_storage<T, 0U>` | Zero-capacity specialization whose accessors return `nullptr`. |

## Usage Example
```cpp
// See: samples/sample_storage.cpp
castle::memory::raw_storage<uint32_t, 2U> storage;
void* first = storage.address(0U);
```

## Constraints & Notes
- `raw_storage<T, N>` only reserves bytes; it does not construct or destroy `T`.
- For `N > 0`, callers must keep indices below `capacity`.
- Reusing a slot for a different object type requires correct alignment and explicit lifetime management.
- `raw_storage<T, 0U>` has no backing bytes and always returns `nullptr`.
