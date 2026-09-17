# Function

## Overview
`function.hpp` provides an owning, small-buffer callable wrapper similar in role to `std::function`, but with fixed inline storage and no heap allocation. Use it when a component needs one erased callable with deterministic storage.

## Header
`#include "castle/callbacks/function.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/config.md`](../core/config.md)
- [`../core/error_handler.md`](../core/error_handler.md)
- [`../core/traits.md`](../core/traits.md)
- [`../core/types.md`](../core/types.md)
- [`../utility/move.md`](../utility/move.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../memory/new.md`](../memory/new.md)

## Public API
| API | Description |
| --- | --- |
| `function<R(Args...), StorageSize, StorageAlignment>` | Owns one callable in an inline byte buffer. |
| `function(callable)` | Stores a callable object by value if it fits in the configured storage. |
| `function(callback_ptr_t)` / `operator=(callback_ptr_t)` | Stores a raw function pointer; `nullptr` produces/keeps an empty wrapper. |
| Copy/move constructor and assignment | Copy or move the stored callable through internal function pointers. |
| `operator bool()` | Returns whether a callable target is present. |
| `operator()(args...)` | Invokes the stored callable; empty invocation triggers `CASTLE_ASSERT`. |

## Usage Example
See `samples/sample_function.cpp`.

```cpp
int total = 0;
castle::callbacks::function<void(int), 32U> callback{
    [&total](int value) noexcept
    {
        total += value;
    }
};
callback(5);
```

## Constraints & Notes
- No heap allocation.
- `StorageSize` and `StorageAlignment` are compile-time limits; oversized or over-aligned callables fail to compile.
- Copy/move support depends on the stored callable's constructors.
- Empty wrappers are allowed and test false with `operator bool()`.
- No internal synchronization; concurrent access requires external coordination.
