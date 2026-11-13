# Temperature

## Overview
Thermodynamic temperature units for `castle::units`: kelvin and millikelvin plus the affine Celsius, Fahrenheit and deci-Celsius scales. Use them in place of raw `double` temperatures so that a function taking `celsius<double>` cannot receive a Fahrenheit value or a raw sensor count.

## Header
`#include "castle/units/temperature.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `kelvin_unit`, `millikelvin_unit` | Offset-free units (`kelvin_unit` is coherent). |
| `celsius_unit` | 1 K steps, offset 273.15 K (`ratio<27315, 100>`). |
| `fahrenheit_unit` | 5/9 K steps, offset `ratio<45967, 180>` K (zero at 459.67 R). |
| `deci_celsius_unit` | `celsius_unit` scaled by 1/10: 0.1 degC steps, the raw format of int16 sensors. |
| `kelvin<Rep>`, `millikelvin<Rep>`, `celsius<Rep>`, `fahrenheit<Rep>`, `deci_celsius<Rep>` | `quantity` aliases for the units. |
| `literals::_K`, `_mK`, `_degC`, `_degF` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/temperature.cpp`.

```cpp
#include "castle/units/temperature.hpp"

using namespace castle::units::literals;

bool heater_on(castle::units::deci_celsius<int16_t> reading, castle::units::celsius<int32_t> setpoint)
{
    return reading < setpoint;
}

const castle::units::deci_celsius<int16_t> raw(253);
const castle::units::celsius<double> room = raw;                              // 25.3 degC
const castle::units::fahrenheit<int32_t> boiling(castle::units::celsius<int32_t>(100)); // explicit: 212 degF
const bool same = (-40_degC == -40_degF);                                     // true
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Affine rules:
  - Conversions apply the origin offset. They are implicit only into a floating representation; integral conversions truncate toward zero and need an explicit constructor or `quantity_cast` (`0 degC` to `kelvin<int>` is 273 K, `100 degC` to `fahrenheit<int>` is 212 degF).
  - Comparisons (`==`, `<`, ...) work across all temperature units.
  - `+` and `-` are fine between two values of the same unit, and between offset-free units (kelvin, millikelvin).
  - Mixing two different offset units in `+` or `-` (for example Celsius and Fahrenheit, or Celsius and deci-Celsius) is rejected at compile time.
  - Offset units cannot be multiplied or divided by quantities, durations or each other, and cannot be inverted; convert to kelvin first. Multiplying by a plain scalar stays within the unit.
  - Differences between temperatures should be taken in kelvin: `kelvin<double>(a) - kelvin<double>(b)`.
- `abs` of an offset unit is relative to the unit's own zero.
