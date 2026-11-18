# Value Inversion

## Overview
Provides a small callable object that reflects values with the arithmetic expression `(offset + minuend) - value`. It exists for deterministic subtractive inversion and range reflection, not reciprocal math.

## Header
`#include "castle/math/invert.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/traits.hpp`](../core/traits.md)
- [`castle/core/types.hpp`](../core/types.md)
- [`castle/core/type_ranges.hpp`](../core/type_ranges.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> class invert` | Stores an offset and minuend for a callable inversion mapping. `T` must be arithmetic. |
| `invert() noexcept` | Default constructor. Uses `offset = 0`; `minuend = 0` for signed arithmetic and `minuend = numeric_limits<T>::max()` for unsigned arithmetic. |
| `invert(T offset, T minuend) noexcept` | Custom constructor for explicit mapping parameters. |
| `T operator()(T value) const noexcept` | Returns `minuend - (value - offset)`. `O(1)`. |
| `T offset() const noexcept` | Returns the stored offset. `O(1)`. |
| `T minuend() const noexcept` | Returns the stored minuend. `O(1)`. |

## Usage Example
```cpp
#include "castle/math/invert.hpp"

castle::math::invert<int> negate;
constexpr int negated = negate(5);

castle::math::invert<unsigned int> reflect(10U, 50U);
constexpr unsigned int mapped = reflect(12U);
```

See [`samples/sample_invert.cpp`](../../samples/sample_invert.cpp).

## Constraints & Notes
- The default signed behavior is numeric negation because the stored minuend is `0`.
- The default unsigned behavior reflects the input around the full unsigned range because the stored minuend is `numeric_limits<T>::max()`.
- No overflow or saturation checks are performed for signed arithmetic; callers must choose offsets, minuends, and inputs that stay within the desired range.
