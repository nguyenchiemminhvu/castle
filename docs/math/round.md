# Round

## Overview
Rounds a floating-point value to the nearest integer and resolves `x.5` ties away from zero (the rule of C `round()`), returning the caller's integer type directly. It exists for quantising physical values onto integer grids such as fixed-point fields, protocol data fields (for example the RTCM [`station_arp`](../castle_ext/protocols/rtcm/messages/station_arp.md) and [`msm`](../castle_ext/protocols/rtcm/messages/msm.md) setters) or ADC/DAC codes, without `<math.h>` and its `long` result type.

## Header
`#include "castle/math/round.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/traits.hpp`](../core/traits.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename Int, typename Float> constexpr Int round_half_away(Float value) noexcept` | Nearest `Int` to `value`; ties away from zero (`2.5 -> 3`, `-2.5 -> -3`). Requires integral `Int` and floating-point `Float`. `O(1)`. |

## Usage Example
```cpp
#include "castle/math/round.hpp"

constexpr int32_t up = castle::math::round_half_away<int32_t>(2.5);    // 3
constexpr int32_t down = castle::math::round_half_away<int32_t>(-2.5); // -3
const int64_t units = castle::math::round_half_away<int64_t>(1.23456 / 0.0001); // 12346
```

See [`samples/math/round.cpp`](../../samples/math/round.cpp).

## Constraints & Notes
- `value` must be finite and the rounded result must fit `Int`; NaN, infinities or out-of-range values make the conversion undefined. Range-check before calling.
- The result is `value +/- 0.5` truncated toward zero, so a value just below a tie by less than half an ULP (for example `0.49999999999999994`) rounds up.
- No allocation, exceptions or hidden state; usable in constant expressions.
