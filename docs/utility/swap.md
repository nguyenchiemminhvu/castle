# Swap

## Overview
`castle::swap` exchanges two objects using Castle move semantics. It exists as a lightweight alternative to `<utility>` for embedded code.

## Header
`#include "castle/utility/swap.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [move.md](move.md)

## Public API
| API | Description |
|---|---|
| `swap(T& lhs, T& rhs)` | Exchanges `lhs` and `rhs` using one stack temporary and move operations. |

## Usage Example
See `samples/sample_swap.cpp`.

```cpp
castle::swap(lhs, rhs);
```

## Constraints & Notes
- No heap allocation.
- Requires move construction, move assignment, and destruction for `T`.
- `noexcept` depends on the moved type's operations.
