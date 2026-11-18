# Pair

## Overview
`castle::pair` stores two heterogeneous values in one fixed-size aggregate. It exists for compact key/value or coordinate-like data where public member access and deterministic behavior are preferred.

## Header
`#include "castle/utility/pair.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [types.md](../core/types.md), [traits.md](../core/traits.md), [forward.md](forward.md), [move.md](move.md), [swap.md](swap.md)

## Public API
| API | Description |
|---|---|
| `pair<T1, T2>` | Two-value aggregate with public `first` and `second` members. |
| `pair::swap(other)` | Swaps both members. |
| `make_pair(first, second)` | Builds a pair with decayed element types. |
| `get<0>(pair)` / `get<1>(pair)` | Accesses `first` or `second` by index. |
| `swap(pair&, pair&)` | Non-member swap wrapper. |
| `operator==`, `!=`, `<`, `>`, `<=`, `>=` | Element-wise equality and lexicographic ordering. |

## Usage Example
See `samples/sample_pair.cpp`.

```cpp
auto item = castle::make_pair(7U, 25U);
auto key = castle::get<0>(item);
```

## Constraints & Notes
- No dynamic allocation or hidden ownership.
- `get` supports only indices `0` and `1`.
- Ordering compares `first` before `second`, matching standard pair semantics.
