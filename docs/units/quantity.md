# Quantity

## Overview
The engine of `castle::units`: `quantity<Unit, Rep>` is one arithmetic value plus a compile-time unit. The unit records the physical dimension (exponents of L, M, T, I, Θ, N, J and plane angle), an exact ratio scale, an optional pi factor, an optional origin offset (for °C/°F), and an optional kind tag. Quantities with different dimensions or kinds cannot be added, compared or assigned. Multiplication and division derive the unit of the result, which turns unit mistakes into compile errors.

## Header
`#include "castle/units/quantity.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [constants](../core/constants.md)
- [traits](../core/traits.md)
- [type_ranges](../core/type_ranges.md)
- [duration](../chrono/duration.md)
- [gcd](../math/gcd.md)
- [lcm](../math/lcm.md)
- [ratio](../math/ratio.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dimension<L, M, T, I, Th, N, J, A>` | Compile-time dimension exponents. `dimension_multiply_t`, `dimension_divide_t` and `dimension_inverse_t` combine them. |
| `dim::dimensionless`, `length`, `mass`, `time`, `current`, `temperature`, `amount`, `luminous_intensity`, `angle` | Base dimensions. Derived ones (`dim::velocity`, ...) live in the quantity headers. |
| `unit<Dim, Ratio = ratio<1>, PiExponent = 0, Offset = ratio<0>, Kind = void>` | Canonical unit; ratios are reduced, so equal units are the same type. `coherent = value * Ratio * pi^PiExponent + Offset`. |
| `scaled_unit_t<Unit, Ratio>` | The same unit scaled by an extra ratio, e.g. a 1e-7 degree fixed-point field. |
| `coherent_unit_t<Unit>` | Scale 1, no pi factor, no offset; same dimension and kind. |
| `quantity<Unit, Rep>` | `value()`, `zero()`, `min()`, `max()`, `+=`, `-=` (same type, lossless conversions apply), `*=`/`/=` by scalar. |
| Constructors | `explicit quantity(Rep)`. Implicit from a compatible quantity when lossless (floating destination, or integral scale factor and offset); explicit when lossy. |
| `quantity_cast<To>(from)` | Explicit conversion between quantities and/or `chrono::duration` of the same dimension; may change the kind. Integral results truncate toward zero. |
| `+`, `-` (binary) | Compatible quantities, evaluated in their common unit. Mixing two different offset units is a compile error. |
| `==`, `!=`, `<`, `<=`, `>`, `>=` | Compatible quantities in their common unit; offsets are honoured, so 0 °C < 33 °F. |
| `q * s`, `s * q`, `q / s`, `s / q` | Arithmetic scalars. `s / q` gives the reciprocal unit. |
| `a * b`, `a / b` | Quantities, or a quantity and a `chrono::duration`. Returns a quantity with the derived unit, a `chrono::duration` in seconds for pure time, or a plain `Rep` in coherent scale when dimensionless. |
| `inverse(q)` | `1 / q` for quantities and durations, e.g. the frequency of a period. |
| `abs(q)` | Magnitude in the same unit. |
| `is_quantity<T>` | Trait. |
| `meta::common_type<quantity, quantity>` | Common unit: gcd/lcm scale when pi factor and offset agree, otherwise the coherent unit with a floating representation. |

## Usage Example
See `samples/units/quantity.cpp`.

```cpp
#include "castle/units/quantity.hpp"

namespace cu = castle::units;
using tick_unit = cu::unit<cu::dim::length, castle::math::ratio<25, 10000>>; // 2.5 mm encoder tick
using speed_unit = cu::unit<cu::dimension_divide_t<cu::dim::length, cu::dim::time>>;

template <typename Rep> using ticks = cu::quantity<tick_unit, Rep>;
template <typename Rep> using meters_per_second = cu::quantity<speed_unit, Rep>;

meters_per_second<double> wheel_speed(ticks<int32_t> delta, castle::chrono::milliseconds window)
{
    return ticks<double>(delta) / window; // rescaled to m/s implicitly
}
```

## Constraints & Notes
- No heap, exceptions, RTTI or virtual dispatch. `sizeof(quantity<U, Rep>) == sizeof(Rep)`, and all unit information is compile-time.
- Arithmetic and conversions are unchecked, like `chrono::duration`. Integral conversions truncate toward zero, and the intermediate product `value * num * den` is evaluated in `intmax_t`.
- Conversions that change the pi factor (degrees and radians) are computed in floating point (`double` for integral reps).
- Offset units are affine. Conversions and comparisons are correct across units, and same-unit `+`/`-` works on the raw values, but different offset units cannot be added or subtracted, and offset units cannot be multiplied or divided. Use kelvin for temperature differences.
- There is no time quantity: time is `chrono::duration` (see [time](time.md)). Chrono's scalar `*`, `/`, `%` accept only arithmetic scalars, so `duration * quantity` reaches the units operators.
- Dimensionless results with kind `void` collapse to `Rep`. Kinded dimensionless quantities (such as decibels) stay quantities.
- Angle is a dimension, so `radius * angle` is `length·angle`, not length. Use `.value()` for arc-length formulas.
