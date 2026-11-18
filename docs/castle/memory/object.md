# Object

## Overview
Converts a raw address back into a typed pointer after a live object has already been created there. It exists to combine pointer reinterpretation with Castle's lifetime laundering helper.

## Header
`#include "castle/memory/object.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Construct](construct.md)
- [Destroy](destroy.md)
- [Lifetime](lifetime.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T> T* castle::memory::object_from_address(void* address)` | Returns a laundered mutable `T*` for storage that already contains a live `T`. O(1). |
| `template <typename T> T const* castle::memory::object_from_address(void const* address)` | Returns a laundered const `T*` for storage that already contains a live `T`. O(1). |

## Usage Example
```cpp
// See: samples/sample_object.cpp
auto* view = castle::memory::object_from_address<MyType>(storage_address);
```

## Constraints & Notes
- `address` must satisfy `alignof(T)`.
- A live `T` object must already occupy the storage before calling either overload.
- This helper does not construct, destroy, or validate the object; it only returns a typed view.
