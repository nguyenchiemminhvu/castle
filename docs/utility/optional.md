# Optional

## Overview
`castle::optional` stores zero or one value in preallocated in-place storage. It exists for embedded code that needs an explicit "maybe present" state without sentinels, dynamic allocation, or exceptions.

## Header
`#include "castle/utility/optional.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [error_handler.md](../core/error_handler.md), [types.md](../core/types.md), [traits.md](../core/traits.md), [construct.md](../memory/construct.md), [destroy.md](../memory/destroy.md), [alignment.md](../memory/alignment.md), [forward.md](forward.md), [move.md](move.md)

## Public API
| API | Description |
|---|---|
| `nullopt_t`, `nullopt` | Tag type and value representing the disengaged state. |
| `optional<T>` | Allocation-free optional container for one `T`. |
| `optional<T>::has_value()`, `operator bool()` | Query whether a value is engaged. |
| `optional<T>::value()`, `operator*()`, `operator->()` | Access the stored value; empty access checks use Castle assertions. |
| `optional<T>::begin()`, `end()`, `cbegin()`, `cend()` | Single-element iterator interface when engaged, otherwise `nullptr`. |
| `optional<T>::reset()`, `emplace(...)`, `value_or(...)`, `swap(...)` | Manage the stored object's lifetime and fallback value. |
| Comparison operators with `optional` and `nullopt` | Compare presence and stored values. |
| `swap(optional<T>&, optional<T>&)` | Non-member swap wrapper. |
| `make_optional(value)` | Creates an engaged optional with decayed value type. |

## Usage Example
See `samples/sample_optional.cpp`.

```cpp
castle::optional<uint32_t> reading;
reading.emplace(42U);
uint32_t value = reading.value_or(0U);
```

## Constraints & Notes
- No heap allocation; storage is embedded inside the optional object.
- `T` must be a non-reference, non-array, destructible object type.
- Empty `value()`/`operator*`/`operator->` access routes through Castle assertion handling.
- Assignment reconstructs the stored object instead of requiring `T` to be assignable.
