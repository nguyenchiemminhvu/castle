# Tuple

## Overview
`castle::tuple` is a fixed-size heterogeneous aggregate implemented without the standard library. It exists for generic embedded code that needs compile-time indexed storage, reference tuples, and type-based element access.

## Header
`#include "castle/utility/tuple.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [error_handler.md](../core/error_handler.md), [types.md](../core/types.md), [traits.md](../core/traits.md), [integral_sequence.md](integral_sequence.md), [move.md](move.md), [forward.md](forward.md)

## Public API
| API | Description |
|---|---|
| `tuple<Ts...>` | Fixed-size heterogeneous aggregate. |
| `tuple::size()` | Returns the number of elements. |
| `tuple_element<Index, Tuple>`, `tuple_element_t<Index, Tuple>` | Maps an index to the corresponding element type. |
| `tuple_size<Tuple>`, `tuple_size_v<Tuple>` | Compile-time tuple length. |
| `get<Index>(tuple)` | Accesses an element by compile-time index. |
| `get<T>(tuple)` | Accesses an element by unique type. |
| `make_tuple(...)` | Creates a tuple with decayed element types. |
| `tie(...)` | Creates a tuple of lvalue references. |
| `ignore_t`, `ignore` | Assignment sink object used with tuple-like code. |
| `forward_as_tuple(...)` | Creates a tuple of forwarding references. |

## Usage Example
See `samples/sample_tuple.cpp`.

```cpp
auto values = castle::make_tuple(10U, 20U);
auto second = castle::get<1>(values);
```

## Constraints & Notes
- No heap allocation, RTTI, virtual dispatch, or Castle-thrown exceptions.
- `get<T>` requires that `T` appear exactly once in the tuple.
- Reference tuples preserve references directly; they do not copy the referents.
