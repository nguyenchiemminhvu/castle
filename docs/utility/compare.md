# Compare

## Overview
This header provides small comparator functors and a `compare` helper that derives all relational operations from a single less-than comparator. It exists so embedded value types can define one ordering primitive and reuse it consistently.

## Header
`#include "castle/utility/compare.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [error_handler.md](../core/error_handler.md), [traits.md](../core/traits.md)

## Public API
| API | Description |
|---|---|
| `less<T>` | Functor that returns `lhs < rhs`. |
| `greater<T>` | Functor that returns `lhs > rhs`. |
| `equal_to<T>` | Functor that returns `lhs == rhs`. |
| `compare<T, TLess>` | Derives `lt`, `gt`, `lte`, `gte`, `eq`, `ne`, and `cmp` from `TLess`. |
| `compare<T, TLess>::cmp_result` | Enumeration with `Less`, `Equal`, and `Greater`. |
| `cmp_3_ways<T, TLess>(lhs, rhs)` | Returns `-1`, `0`, or `1` using `compare<T, TLess>::cmp`. |

## Usage Example
See `samples/sample_compare.cpp`.

```cpp
using order = castle::compare<int>;
bool is_before = order::lt(1, 2);
int relation = castle::cmp_3_ways<int>(2, 1);
```

## Constraints & Notes
- No allocation or runtime state.
- `TLess` must be callable as `bool(const T&, const T&)`.
- Equality is derived from the less-than relation, not from `operator==`.
