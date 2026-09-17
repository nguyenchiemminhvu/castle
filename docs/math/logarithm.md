# Logarithm

## Overview
Compile-time integer logarithm templates for deriving bit widths, digit counts, and radix-dependent constants without runtime math.

## Header
`#include "castle/math/logarithm.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)

## Public API
| API | Description |
|---|---|
| `template <size_type Value, size_type Base> struct castle::math::logarithm` | Computes `floor(log_Base(Value))` at compile time. The primary template requires `Value > 0` and `Base > 1`. |
| `template <size_type Base> struct castle::math::logarithm<1U, Base>` | Specialization returning `0` for an input value of `1`. |
| `template <size_type Base> struct castle::math::logarithm<0U, Base>` | Specialization returning `0` for an input value of `0`. |
| `template <size_type Value> struct castle::math::log2` | Convenience wrapper for `logarithm<Value, 2U>`. Requires `Value > 0`. |
| `template <size_type Value> struct castle::math::log10` | Convenience wrapper for `logarithm<Value, 10U>`. Requires `Value > 0`. |

## Usage Example
See `samples/sample_logarithm.cpp` for a complete example.

```cpp
static_assert(castle::math::logarithm<81U, 3U>::value == 4U, "");
static_assert(castle::math::log2<1024U>::value == 10U, "");
static_assert(castle::math::log10<999U>::value == 2U, "");
```

## Constraints & Notes
- Results are rounded down to the nearest integer exponent.
- `log2` and `log10` reject zero at compile time; only the direct `logarithm<0U, Base>` specialization yields `0`.
- Complexity is `O(log_Base(Value))` template recursion.
