# Charge

## Overview
Electric charge units for `castle::units`: the coulomb, the millicoulomb and the battery-style ampere-hour and milliampere-hour. Use them in place of raw `double` capacities so that a function taking `milliampere_hours<double>` cannot receive a current, a voltage, or a value in the wrong scale.

## Header
`#include "castle/units/charge.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dim::charge` | Charge dimension, `T I` (`dimension<0, 0, 1, 1, 0, 0, 0, 0>`). |
| `coulomb_unit`, `millicoulomb_unit` | SI charge units (`coulomb_unit` is coherent, 1 A s). |
| `ampere_hour_unit`, `milliampere_hour_unit` | Exact definitions: 3600 C and 3.6 C. |
| `coulombs<Rep>`, `millicoulombs<Rep>`, `ampere_hours<Rep>`, `milliampere_hours<Rep>` | `quantity` aliases for the units above. |
| `literals::_C`, `_mC`, `_Ah`, `_mAh` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/charge.cpp`.

```cpp
#include "castle/units/charge.hpp"
#include "castle/units/time.hpp"

using namespace castle::units::literals;

// Coulomb counting: current times a duration is a charge.
castle::units::coulombs<double> used = discharge_current * 60.0_s;
remaining = remaining - used;                                 // milliampere_hours<double>

const castle::units::milliampere_hours<int32_t> pack = castle::units::ampere_hours<int32_t>(3); // exact: 3000 mAh
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Current times a duration gives a charge quantity in the combined scale (for example amperes times hours is ampere-hours-sized coulombs); a charge divided by a duration gives a current quantity. Convert the result to the unit you need with an implicit or explicit conversion.
- `1 mAh = 3.6 C` is not an integer number of coulombs, so integral milliampere-hours to coulombs truncates and needs an explicit constructor or `quantity_cast`; coulombs to millicoulombs and ampere-hours to milliampere-hours are exact.
- `_mC` is millicoulombs; `_C` is coulombs (and is unrelated to Celsius).
