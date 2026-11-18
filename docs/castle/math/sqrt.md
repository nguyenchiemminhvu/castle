# Sqrt

## Overview
Compile-time integer floor square root for sizing and discrete math, implemented as a deterministic binary search.

## Header
`#include "castle/math/sqrt.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)

## Public API
| API | Description |
|---|---|
| `template <size_type Value, size_type Root = 1U> struct castle::sqrt` | Computes the largest integer `r` such that `r * r <= Value`. |
| `castle::sqrt<Value>::value` | Floor square-root result. |
| `castle::sqrt<Value>::value_type` | Alias for `size_type`. |
| `castle::sqrt<Value>::type` | `meta::integral_constant<size_type, value>` wrapper around the result. |

## Usage Example
See `samples/sample_sqrt.cpp` for a complete example.

```cpp
static_assert(castle::sqrt<144U>::value == 12U, "");
static_assert(castle::sqrt<145U>::value == 12U, "");
```

## Constraints & Notes
- The template lives in namespace `castle`, not `castle::math`.
- Results are rounded down.
- The optional `Root` parameter is an internal lower-bound hint for the search.
