# Absolute Value

## Overview
Deterministic absolute-value helpers for signed integers, unsigned integers, and floating-point values. Use `abs()` when the result may stay in the original type, and `uabs()` when you need a full-range unsigned magnitude that can represent the signed minimum value.

## Header
`#include "castle/math/abs.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/error_handler.hpp`](../core/error_handler.md)
- [`castle/core/traits.hpp`](../core/traits.md)
- [`castle/core/type_ranges.hpp`](../core/type_ranges.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> T abs(T value) noexcept` | Signed-integral overload. Returns `|value|` in the same type and routes `min()` through Castle's error handler because the positive counterpart is not representable. `O(1)`. |
| `template <typename T> T abs(T value) noexcept` | Floating-point overload. Returns `-value` when `value < 0`, otherwise `value`. `O(1)`. |
| `template <typename T> T abs(T value) noexcept` | Unsigned overload. Identity operation. `O(1)`. |
| `template <typename T> typename meta::make_unsigned<T>::type uabs(T value) noexcept` | Signed-integral overload. Returns the magnitude as the unsigned counterpart of `T`, including for `min()`. `O(1)`. |
| `template <typename T> T uabs(T value) noexcept` | Unsigned overload. Identity operation. `O(1)`. |

## Usage Example
```cpp
#include "castle/math/abs.hpp"

constexpr int a = castle::math::abs(-7);
constexpr unsigned int b =
    castle::math::uabs(castle::numeric_limits<int>::min());
```

See [`samples/sample_abs.cpp`](../../samples/sample_abs.cpp).

## Constraints & Notes
- `abs()` for signed integers cannot represent `numeric_limits<T>::min()` in the same type; Castle routes that case through the configured error handling path.
- `uabs()` is the safe full-range choice for signed integrals because the return type is unsigned.
- Floating-point `abs()` uses a comparison-and-negation implementation; on IEEE-754 targets, `-0.0` compares equal to zero and is therefore returned unchanged.
- No STL math facilities are required.
