# SOO Buffer

## Overview
Stores one object inside a fixed inline buffer. It exists for very small object storage paths that must stay heap-free and deterministic, but it still relies on careful caller-managed lifetime rules.

## Header
`#include "castle/memory/soo_buffer.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Traits](../core/traits.md)
- [Types](../core/types.md)
- [Utility](../utility/utility.md)
- [Lifetime](lifetime.md)
- [New](new.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T, size_type StackSize = sizeof(T)> class castle::memory::soo_buffer` | Inline buffer type aligned for `T` and storing one object. |
| `soo_buffer(Arg&& arg)` | Constructs an object in the inline storage. Enabled only when `sizeof(T) <= StackSize`. |
| `soo_buffer(soo_buffer const&) = delete` | Copy construction is disabled. |
| `soo_buffer& operator=(soo_buffer const&) = delete` | Copy assignment is disabled. |
| `soo_buffer(soo_buffer&& other)` | Move constructor that destroys `*this`, moves from `other`, then destroys `other`. |
| `soo_buffer& operator=(soo_buffer&& other)` | Move assignment with the same destroy/move/destroy sequence. |
| `~soo_buffer()` | Destroys the object currently assumed to live in the buffer. |
| `T* get()` / `T const* get() const` | Returns a laundered pointer to the inline object. O(1). |

## Usage Example
```cpp
// See: samples/sample_soo_buffer.cpp
castle::memory::soo_buffer<MyType> buffer(MyType(7U));
MyType* ptr = buffer.get();
```

## Constraints & Notes
- `StackSize` must be at least `sizeof(T)` for the constructor to participate.
- The constructor placement-news the deduced `Arg`, then later accessors and destruction treat the bytes as `T`. In practice, pass an rvalue whose deduced type is exactly `T`.
- `get()` and the destructor assume a live `T` currently occupies the inline bytes.
- The move members explicitly call buffer destructors; use them only if surrounding lifetime management makes those direct destructor calls valid and non-repeating.
