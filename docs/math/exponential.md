# Exponential

## Overview
Thin, allocation-free wrappers for the natural exponential and natural logarithm over `float`, `double`, and `long double`. They mirror the layout of the trigonometric wrappers in `castle/math/linalg/trigonometry.hpp`, forward to the platform C math library, and serve as the scalar building blocks of [`castle::math::complex`](complex.md).

## Header
`#include "castle/math/exponential.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)

## Public API
| Signature | Description |
| --- | --- |
| `float exp(float value) noexcept` | Returns `e` raised to `value` via `expf(value)`. `O(1)`. |
| `double exp(double value) noexcept` | Returns `e` raised to `value` via `exp(value)`. `O(1)`. |
| `long double exp(long double value) noexcept` | Returns `e` raised to `value` via `expl(value)`. `O(1)`. |
| `float log(float value) noexcept` | Returns the natural logarithm via `logf(value)`. Zero yields `-inf`, negative input yields NaN. `O(1)`. |
| `double log(double value) noexcept` | Returns the natural logarithm via `log(value)`. Zero yields `-inf`, negative input yields NaN. `O(1)`. |
| `long double log(long double value) noexcept` | Returns the natural logarithm via `logl(value)`. Zero yields `-inf`, negative input yields NaN. `O(1)`. |

All functions live in `castle::math` and are `CASTLE_INLINE` (not `constexpr`).

## Usage Example
```cpp
#include "castle/math/exponential.hpp"

const double e1 = castle::math::exp(1.0);
const double one = castle::math::log(e1);
```

See [`samples/math/exponential.cpp`](../../samples/math/exponential.cpp).

## Constraints & Notes
- Precision and determinism are those of the platform C math library.
- The overloads are not `constexpr`, so they cannot be used in constant expressions.
- `log()` of zero or a negative value follows C library semantics (`-inf` / NaN); no assertion is raised by these scalar wrappers.
- No `int` overload exists: integral arguments are ambiguous between the three floating-point overloads and must be cast explicitly.
