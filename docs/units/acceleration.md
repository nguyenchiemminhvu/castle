# Acceleration

## Overview
Acceleration units for `castle::units`: meters per second squared, standard gravity (and its milli multiple, as reported by IMUs) and feet per second squared. Use them in place of raw `double` accelerations so that a function taking `meters_per_second_squared<double>` cannot receive a speed or a value in the wrong scale.

## Header
`#include "castle/units/acceleration.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)

## Public API
| API | Description |
|---|---|
| `dim::acceleration` | Dimension `L / T^2`. |
| `meter_per_second_squared_unit` | Coherent acceleration unit. |
| `standard_gravity_unit`, `milli_standard_gravity_unit`, `foot_per_second_squared_unit` | 9.80665 m/s^2, 0.00980665 m/s^2, 0.3048 m/s^2 (exact definitions). |
| `meters_per_second_squared<Rep>`, `standard_gravities<Rep>`, `milli_standard_gravities<Rep>`, `feet_per_second_squared<Rep>` | `quantity` aliases. |
| `literals::_mps2`, `_gn`, `_mgn` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/acceleration.cpp`.

```cpp
#include "castle/units/acceleration.hpp"
#include "castle/units/time.hpp"
#include "castle/units/velocity.hpp"

using namespace castle::units::literals;

castle::units::meters_per_second_squared<double> to_si(castle::units::milli_standard_gravities<int32_t> raw)
{
    return castle::units::meters_per_second_squared<double>(raw);
}

const auto g = to_si(castle::units::milli_standard_gravities<int32_t>(1000));   // 9.80665 m/s^2
const castle::units::meters_per_second_squared<double> accel = (30.0_mps - 10.0_mps) / 4.0_s; // 5 m/s^2
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Standard gravity is exactly 9.80665 m/s^2.
- A velocity divided by a coherent duration gives `meters_per_second_squared`; `acceleration * duration` gives a velocity.
- With mixed scales (for example `km/h / s`) the result stays in a scaled unit; convert to the coherent unit to read it in SI.
- Integral conversion from `standard_gravities` to `milli_standard_gravities` is exact; the reverse truncates and needs an explicit constructor.
