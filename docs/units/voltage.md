# Voltage

## Overview
Electric potential units for `castle::units`: SI multiples of the volt. Use them in place of raw ADC numbers so that a function taking `volts<double>` cannot receive a current, a resistance, or millivolts mistaken for volts.

## Header
`#include "castle/units/voltage.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dim::voltage` | `dimension<2, 1, -3, -1, 0, 0, 0, 0>` (M L^2 T^-3 I^-1). |
| `volt_unit`, `kilovolt_unit`, `millivolt_unit`, `microvolt_unit` | SI voltage units (`volt_unit` is coherent). |
| `volts<Rep>`, `kilovolts<Rep>`, `millivolts<Rep>`, `microvolts<Rep>` | `quantity` aliases for the voltage units. |
| `literals::_V`, `_kV`, `_mV`, `_uV` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/voltage.cpp`.

```cpp
#include "castle/units/voltage.hpp"

using namespace castle::units::literals;

bool battery_low(castle::units::volts<double> cell)
{
    return cell < castle::units::volts<double>(3.3);
}

const castle::units::millivolts<int32_t> reading = adc_read(2730U, 3000_mV); // 2000 mV
const castle::units::volts<double> volts = reading;                          // implicit: 2.0 V
const castle::units::volts<int32_t> whole(castle::units::millivolts<int32_t>(3300)); // explicit: 3 V
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Conversions to a coarser integral unit (for example millivolts to volts) truncate toward zero and therefore need an explicit constructor or `quantity_cast`.
- `volts * amperes` gives a power quantity (M L^2 T^-3); `volts / amperes` gives a resistance (see [resistance](resistance.md)).
