# Lifetime

## Overview
Provides `launder` helpers for pointers that refer to objects created or recreated in raw storage. It exists to keep C++17 lifetime rules explicit without depending on the STL `std::launder` interface.

## Header
`#include "castle/memory/lifetime.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T> T* castle::memory::launder(T* pointer)` | Returns a lifetime-correct mutable pointer to the active object at the same address. O(1). |
| `template <typename T> T const* castle::memory::launder(T const* pointer)` | Returns a lifetime-correct const pointer to the active object at the same address. O(1). |

## Usage Example
```cpp
// See: samples/sample_lifetime.cpp
auto* view = castle::memory::launder(existing_pointer);
```

## Constraints & Notes
- `pointer` should refer to an object whose lifetime is already active.
- This is most relevant after reconstructing an object in reused storage with placement `new`.
- The helper does not allocate, relocate, or destroy anything; it only adjusts pointer access semantics.
