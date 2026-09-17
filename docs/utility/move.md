# Move

## Overview
`castle::move` casts an expression to an rvalue reference of its base type. It exists so Castle code can express move intent without depending on `<utility>`.

## Header
`#include "castle/utility/move.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [traits.md](../core/traits.md)

## Public API
| API | Description |
|---|---|
| `move(T&& value)` | Casts `value` to `remove_reference_t<T>&&`. |

## Usage Example
See `samples/sample_move.cpp`.

```cpp
Packet next(castle::move(current));
```

## Constraints & Notes
- Pure cast helper; it does not transfer ownership by itself.
- No allocation, exceptions, RTTI, or virtual dispatch.
- The moved-from object's later state depends on its type's move constructor/assignment.
