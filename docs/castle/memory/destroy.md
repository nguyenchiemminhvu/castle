# Destroy

## Overview
Ends object lifetime explicitly in caller-managed storage. It exists to pair with placement construction in embedded containers and buffers that never use dynamic allocation.

## Header
`#include "castle/memory/destroy.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Traits](../core/traits.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T> void castle::memory::destroy_at(T* pointer)` | Destroys the object at `pointer` when `T` is non-trivially destructible; trivial types are ignored. O(1). |
| `template <typename T> void castle::memory::destroy_n(T* pointer, size_t count)` | Destroys `count` consecutive live objects in reverse order. O(count). |

## Usage Example
```cpp
// See: samples/sample_destroy.cpp
castle::memory::destroy_at(pointer);
castle::memory::destroy_n(first, count);
```

## Constraints & Notes
- For non-trivial `T`, `pointer` must refer to a live object whose lifetime should end exactly once.
- `destroy_n` assumes `[pointer, pointer + count)` contains live `T` objects.
- No deallocation happens here; these helpers only run destructors.
