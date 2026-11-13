# Velocity

## Overview
Speed units for `castle::units`: meters per second plus kilometers per hour, miles per hour, knots, feet per second and the centimeter/millimeter per second multiples. Use them in place of raw `double` speeds so that a function taking `meters_per_second<double>` cannot receive a length or a value in the wrong scale.

## Header
`#include "castle/units/velocity.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dim::velocity` | Dimension `L / T`. |
| `meter_per_second_unit`, `centimeter_per_second_unit`, `millimeter_per_second_unit` | SI speed units (`meter_per_second_unit` is coherent). |
| `kilometer_per_hour_unit`, `mile_per_hour_unit`, `knot_unit`, `foot_per_second_unit` | 5/18 m/s, 0.44704 m/s, 1852/3600 m/s, 0.3048 m/s (exact definitions). |
| `meters_per_second<Rep>`, `kilometers_per_hour<Rep>`, `miles_per_hour<Rep>`, `knots<Rep>`, `feet_per_second<Rep>`, `centimeters_per_second<Rep>`, `millimeters_per_second<Rep>` | `quantity` aliases. |
| `literals::_mps`, `_kmph`, `_mph`, `_kn`, `_cmps`, `_mmps` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. There is no literal for feet per second. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/velocity.cpp`.

```cpp
#include "castle/units/distance.hpp"
#include "castle/units/time.hpp"
#include "castle/units/velocity.hpp"

using namespace castle::units::literals;

bool over_limit(castle::units::meters_per_second<double> speed, castle::units::meters_per_second<double> limit)
{
    return speed > limit;
}

const bool fast = over_limit(100.0_kmph, 25.0_mps);                            // true
const castle::units::meters_per_second<double> speed = 100.0_m / 4.0_s;        // 25 m/s
const castle::units::meters<double> travelled = speed * 10.0_s;                // 250 m
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- A length divided by a coherent duration gives `meters_per_second`; `velocity * duration` gives `meters`.
- With mixed scales (for example `km/h * s`) the result stays in a scaled unit; convert to the coherent unit to read it in SI.
- Conversions to a coarser integral unit truncate toward zero and therefore need an explicit constructor or `quantity_cast`.
