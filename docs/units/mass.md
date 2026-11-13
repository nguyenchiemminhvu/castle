# Mass

## Overview
Mass units for `castle::units`: the kilogram (the coherent SI unit), its metric multiples and the avoirdupois pound and ounce. Use them in place of raw `double` masses so that a function taking `kilograms<double>` cannot receive a force or a value in the wrong scale.

## Header
`#include "castle/units/mass.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `kilogram_unit`, `gram_unit`, `milligram_unit`, `tonne_unit` | Metric mass units (`kilogram_unit` is coherent; grams are `scaled_unit_t` of it). |
| `pound_unit`, `ounce_unit` | Exact international definitions: 0.45359237 kg and 1/16 lb. |
| `kilograms<Rep>`, `grams<Rep>`, `milligrams<Rep>`, `tonnes<Rep>` | `quantity` aliases for the metric units. |
| `pounds<Rep>`, `ounces<Rep>` | `quantity` aliases for the avoirdupois units. |
| `literals::_kg`, `_g`, `_mg`, `_t`, `_lb`, `_oz` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/mass.cpp`.

```cpp
#include "castle/units/mass.hpp"

using namespace castle::units::literals;

castle::units::kilograms<double> net_weight(castle::units::kilograms<double> gross,
                                            castle::units::kilograms<double> tare)
{
    return gross - tare;
}

const castle::units::kilograms<double> net = net_weight(12.5_kg, 250.0_g);                      // 12.25 kg
const castle::units::grams<int32_t> cargo = castle::units::kilograms<int32_t>(3);              // exact: 3000 g
const castle::units::kilograms<int32_t> rounded(castle::units::grams<int32_t>(3999));          // explicit: 3 kg
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- The kilogram is the coherent unit, so the base SI prefix scale is applied to the kilogram: `gram_unit` is `scaled_unit_t<kilogram_unit, milli>`.
- Conversions to a coarser integral unit (for example grams to kilograms) truncate toward zero and therefore need an explicit constructor or `quantity_cast`.
- `_t` is the metric tonne (1000 kg); `_mg` is milligrams.
- Multiplying two masses gives an `M^2` quantity with the combined scale.
