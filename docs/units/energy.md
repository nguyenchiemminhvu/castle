# Energy

## Overview
Energy units for `castle::units`: the joule, its SI multiples and the watt-hour family used for battery capacities. Use them in place of raw `double` energies so that a function taking `joules<double>` cannot receive a power or a value in the wrong scale.

## Header
`#include "castle/units/energy.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dim::energy` | Derived dimension `dimension<2, 1, -2, 0, 0, 0, 0, 0>` (L^2 M T^-2), declared in this header. |
| `joule_unit`, `millijoule_unit`, `kilojoule_unit`, `megajoule_unit` | SI energy units (`joule_unit` is coherent). |
| `milliwatt_hour_unit`, `watt_hour_unit`, `kilowatt_hour_unit` | Exact: 3.6 J, 3600 J, 3.6 MJ. |
| `joules<Rep>`, `millijoules<Rep>`, `kilojoules<Rep>`, `megajoules<Rep>` | `quantity` aliases for the SI units. |
| `milliwatt_hours<Rep>`, `watt_hours<Rep>`, `kilowatt_hours<Rep>` | `quantity` aliases for the watt-hour units. |
| `literals::_J`, `_mJ`, `_kJ`, `_MJ`, `_mWh`, `_Wh`, `_kWh` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/energy.cpp`.

```cpp
#include "castle/units/distance.hpp"
#include "castle/units/energy.hpp"
#include "castle/units/force.hpp"

using namespace castle::units::literals;

castle::units::joules<double> remaining(castle::units::joules<double> capacity,
                                        castle::units::joules<double> used)
{
    return capacity - used;
}

const castle::units::joules<double> left = remaining(castle::units::watt_hours<double>(3.0), 2.0_kJ); // 8800 J
const castle::units::milliwatt_hours<int32_t> pack = castle::units::watt_hours<int32_t>(2);           // exact: 2000 mWh
const castle::units::joules<double> work = 2.0_N * 3.0_m;                                              // 6 J
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Force times length in coherent units is exactly `joules<Rep>`.
- `milliwatt_hour_unit` is 18/5 J, so converting an integral joule value to `milliwatt_hours` (or the reverse) is not exact and needs an explicit constructor or `quantity_cast`; `watt_hours` to `milliwatt_hours` is exact.
- `_mJ` is millijoules and `_MJ` megajoules; `_mWh`, `_Wh`, `_kWh` are watt-hour units.
