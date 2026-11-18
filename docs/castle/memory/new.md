# New

## Overview
Supplies placement-`new` support for Castle's explicit lifetime utilities without introducing heap allocation. It exists so the library can work in hosted and non-hosted environments, including targets where `<new>` is unavailable.

## Header
`#include "castle/memory/new.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Config](../core/config.md)

## Public API
| API | Description |
| --- | --- |
| `#include <new>` or fallback placement operators | Provides placement `new`, placement `new[]`, and matching placement `delete` overloads needed for in-place construction. |
| `void* operator new(size_t, void* p) noexcept` | Fallback scalar placement `new` that returns `p` unchanged when `<new>` is unavailable. O(1). |
| `void* operator new[](size_t, void* p) noexcept` | Fallback array placement `new[]` that returns `p` unchanged when `<new>` is unavailable. O(1). |
| `void operator delete(void*, void*) noexcept` | Fallback matching placement `delete` for failed scalar construction. O(1). |
| `void operator delete[](void*, void*) noexcept` | Fallback matching placement `delete[]` for failed array construction. O(1). |

## Usage Example
```cpp
// See: samples/sample_new.cpp
alignas(uint32_t) unsigned char bytes[sizeof(uint32_t)];
uint32_t* value = ::new (static_cast<void*>(bytes)) uint32_t(7U);
```

## Constraints & Notes
- This header does not provide heap allocation.
- The caller must supply suitably aligned and sufficiently large storage.
- Objects constructed with placement `new` must be destroyed explicitly when they have non-trivial destructors.
