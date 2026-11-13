# Power

## Overview
Power units for `castle::units`: the watt and its SI multiples, plus the dBm absolute power level with conversions to and from linear power. Use them in place of raw `double` powers so that a function taking `watts<double>` cannot receive an energy or a value in the wrong scale.

## Header
`#include "castle/units/power.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [gain](gain.md) (`decibels`, C math `pow`/`log10` wrappers)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dim::power` | Derived dimension `dimension<2, 1, -3, 0, 0, 0, 0, 0>` (L^2 M T^-3), declared in this header. |
| `watt_unit`, `microwatt_unit`, `milliwatt_unit`, `kilowatt_unit`, `megawatt_unit` | SI power units (`watt_unit` is coherent). |
| `watts<Rep>`, `microwatts<Rep>`, `milliwatts<Rep>`, `kilowatts<Rep>`, `megawatts<Rep>` | `quantity` aliases. |
| `literals::_uW`, `_mW`, `_W`, `_kW`, `_MW` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |
| `power_level_kind` | Kind tag separating absolute power levels from relative gain and plain numbers. |
| `decibel_milliwatt_unit`, `decibel_milliwatts<Rep>` | dBm: `10 * log10(P / 1 mW)`. Dimensionless quantity of kind `power_level_kind`. |
| `milliwatts_from_dbm(level)` | `milliwatts<R>` equal to `10^(dBm / 10)`; `R` is `Rep` if floating, otherwise `double`. Converts implicitly to any floating power unit. |
| `dbm_from_power(power)` | `decibel_milliwatts<R>` equal to `10 * log10(P / 1 mW)` for any power unit; `R` is `Rep` if floating, otherwise `double`. |
| `level ± gain`, `gain + level` | dBm plus or minus any gain-kind quantity (`decibels`, `bels`) gives `decibel_milliwatts`. |
| `level - level` | Difference of two dBm levels gives `decibels`. |
| `literals::_dBm` | Floating literal gives `Rep = double`; integer literal gives `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/power.cpp`.

```cpp
#include "castle/units/energy.hpp"
#include "castle/units/power.hpp"
#include "castle/units/time.hpp"

using namespace castle::units::literals;

bool fits_budget(castle::units::watts<double> load, castle::units::watts<double> budget)
{
    return load <= budget;
}

const castle::units::watts<double> load = 1.5_W + 250.0_mW;                                          // 1.75 W
const castle::units::watts<double> average = 100.0_J / 4.0_s;                                        // 25 W
const castle::units::watt_hours<double> used = 2.0_W * castle::units::hours<double>(1.5);           // 3 Wh

const castle::units::watts<double> tx = castle::units::milliwatts_from_dbm(30.0_dBm);                // 1 W = 1000 mW
const castle::units::decibel_milliwatts<double> level = castle::units::dbm_from_power(1.0_W);        // 30 dBm
const castle::units::decibel_milliwatts<double> rx = 30.0_dBm - 3.0_dB + 15.0_dB;                   // 42 dBm
const castle::units::decibels<double> path = 30.0_dBm - 10.0_dBm;                                   // 20 dB
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Energy divided by a `chrono::duration` gives `watts`; watts multiplied by a duration gives energy with the combined scale (watts times hours is `watt_hours`, convertible to `joules`).
- `_mW` is milliwatts and `_MW` megawatts; `_uW` is microwatts.
- Conversions to a coarser integral unit truncate toward zero and therefore need an explicit constructor or `quantity_cast`.
- dBm is logarithmic, so it is not a scaled watt: `decibel_milliwatts` never converts implicitly or explicitly to `watts`, `decibels` or a plain number. Leave the logarithmic domain with `milliwatts_from_dbm` and enter it with `dbm_from_power`.
- Adding two dBm levels does not compile (the operator is deleted); add the linear powers instead. The member `+=` of `quantity` still accepts a level of the same type, so prefer `level + gain`.
- `dbm_from_power` needs a strictly positive power; `0 W` yields `-inf` and a negative power NaN (not checked).
- Including this header pulls in `<math.h>` through gain.hpp.
