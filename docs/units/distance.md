# Distance

## Overview
Length units for `castle::units`: SI multiples of the meter and the common imperial and nautical units. Use them in place of raw `double` lengths so that a function taking `meters<double>` cannot receive a speed, an angle, or a value in the wrong scale.

## Header
`#include "castle/units/distance.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `meter_unit`, `kilometer_unit`, `centimeter_unit`, `millimeter_unit`, `micrometer_unit`, `nanometer_unit` | SI length units (`meter_unit` is coherent). |
| `inch_unit`, `foot_unit`, `yard_unit`, `mile_unit`, `nautical_mile_unit` | Exact international definitions: 0.0254 m, 0.3048 m, 0.9144 m, 1609.344 m, 1852 m. |
| `meters<Rep>`, `kilometers<Rep>`, `centimeters<Rep>`, `millimeters<Rep>`, `micrometers<Rep>`, `nanometers<Rep>` | `quantity` aliases for the SI units. |
| `inches<Rep>`, `feet<Rep>`, `yards<Rep>`, `miles<Rep>`, `nautical_miles<Rep>` | `quantity` aliases for the imperial and nautical units. |
| `literals::_m`, `_km`, `_cm`, `_mm`, `_um`, `_nm`, `_in`, `_ft`, `_yd`, `_mi`, `_nmi` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/distance.cpp`.

```cpp
#include "castle/units/distance.hpp"

using namespace castle::units::literals;

castle::units::meters<double> add_margin(castle::units::meters<double> range)
{
    return range + castle::units::centimeters<double>(50.0);
}

const castle::units::meters<double> padded = add_margin(2.0_km);    // 2000.5 m
const castle::units::millimeters<int32_t> gap = castle::units::centimeters<int32_t>(12); // exact: 120 mm
const castle::units::centimeters<int32_t> coarse(castle::units::millimeters<int32_t>(129)); // explicit: 12 cm
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Conversions to a coarser integral unit (for example millimeters to centimeters) truncate toward zero and therefore need an explicit constructor or `quantity_cast`.
- `_nm` is nanometers; nautical miles use `_nmi`.
- Multiplying two lengths gives an area quantity (`L^2`) with the combined scale.
