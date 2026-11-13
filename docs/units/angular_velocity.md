# Angular Velocity

## Overview
Angular velocity units for `castle::units`: radians, degrees and revolutions per unit time. Use them in place of raw `double` rotation rates so that a function taking `degrees_per_second<double>` cannot receive a frequency or a value in the wrong scale.

## Header
`#include "castle/units/angular_velocity.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dim::angular_velocity` | Dimension `angle / T`; deliberately different from `dim::frequency`. |
| `radian_per_second_unit`, `milliradian_per_second_unit` | rad/s (coherent) and mrad/s. |
| `degree_per_second_unit` | pi / 180 rad/s. |
| `revolution_per_second_unit`, `revolution_per_minute_unit` | 2 pi rad/s and pi / 30 rad/s. |
| `radians_per_second<Rep>`, `milliradians_per_second<Rep>`, `degrees_per_second<Rep>`, `revolutions_per_second<Rep>`, `revolutions_per_minute<Rep>` | `quantity` aliases for the units. |
| `literals::_radps`, `_mradps`, `_degps`, `_rps`, `_rpm` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/angular_velocity.cpp`.

```cpp
#include "castle/units/angle.hpp"
#include "castle/units/angular_velocity.hpp"
#include "castle/units/time.hpp"

using namespace castle::units::literals;

bool within_limit(castle::units::degrees_per_second<double> speed)
{
    return speed <= castle::units::degrees_per_second<double>(3600.0);
}

const bool fast = !within_limit(3000.0_rpm);                                 // 18000 deg/s
const auto speed = castle::units::revolutions<double>(120.0) / castle::units::minutes<double>(1.0); // 120 rpm
const auto turned = castle::units::degrees_per_second<double>(45.0) * castle::units::seconds<double>(2.0); // 90 deg
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Degree and revolution based units carry an exact pi factor. Conversions between them (`rpm`, `rps`, `deg/s`) are exact for integers; conversions to or from rad/s are evaluated in floating point, so an integral rad/s destination needs an explicit constructor or `quantity_cast`.
- `angle / duration` yields the matching angular velocity unit (`degrees / seconds` is `degrees_per_second`); `angular velocity * duration` yields an angle.
- Angular velocity and frequency are different dimensions: `revolutions_per_minute` never converts to `hertz`, in either direction.
- Conversions to a coarser integral unit truncate toward zero and need an explicit constructor or `quantity_cast`.
