# Fibonacci

## Overview
Computes Fibonacci numbers as `castle::size_type` using either a compile-time template form or a runtime `constexpr` function. It exists for deterministic sequence generation without recursion at runtime.

## Header
`#include "castle/math/fib.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/traits.hpp`](../core/traits.md)
- [`castle/core/types.hpp`](../core/types.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <castle::size_type N> castle::size_type fib() noexcept` | Returns the zero-based `N`th Fibonacci number at compile time. Uses an iterative loop after handling `0` and `1`. `O(N)`. |
| `castle::size_type fib(castle::size_type n) noexcept` | Returns the zero-based `n`th Fibonacci number for a runtime index. `O(n)`. |

## Usage Example
```cpp
#include "castle/math/fib.hpp"

constexpr castle::size_type fixed = castle::math::fib<10>();
constexpr castle::size_type runtime = castle::math::fib(7);
```

See [`samples/sample_fib.cpp`](../../samples/sample_fib.cpp).

## Constraints & Notes
- `fib<0>()` and `fib(0)` return `0`; `fib<1>()` and `fib(1)` return `1`.
- Both overloads use `castle::size_type`, so no negative indices are possible.
- No overflow detection is performed; large indices wrap according to `size_type` arithmetic.
