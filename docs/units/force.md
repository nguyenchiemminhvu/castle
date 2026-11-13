# Force

## Overview
Force units for `castle::units`: the newton, its SI multiples and the pound-force. Use them in place of raw `double` forces so that a function taking `newtons<double>` cannot receive a mass or a value in the wrong scale.

## Header
`#include "castle/units/force.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dim::force` | Derived dimension `dimension<1, 1, -2, 0, 0, 0, 0, 0>` (L M T^-2), declared in this header. |
| `newton_unit`, `millinewton_unit`, `kilonewton_unit` | SI force units (`newton_unit` is coherent). |
| `pound_force_unit` | Exactly 4.4482216152605 N (0.45359237 kg * 9.80665 m/s^2). |
| `newtons<Rep>`, `millinewtons<Rep>`, `kilonewtons<Rep>`, `pounds_force<Rep>` | `quantity` aliases. |
| `literals::_N`, `_mN`, `_kN`, `_lbf` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/force.cpp`.

```cpp
#include "castle/units/force.hpp"

using namespace castle::units::literals;

bool within_limit(castle::units::newtons<double> demand, castle::units::newtons<double> limit)
{
    return demand <= limit;
}

const castle::units::newtons<double> thrust = 1.2_kN + 300.0_N;                         // 1500 N
const bool ok = within_limit(thrust, 2.0_kN);                                           // true
const castle::units::millinewtons<int32_t> fine = castle::units::newtons<int32_t>(2);  // exact: 2000 mN
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Mass times acceleration (`L T^-2`) in coherent units is exactly `newtons<Rep>`.
- The pound-force ratio is stored reduced (`8896443230521/2000000000000`). Mixed integral arithmetic with it runs on a 1/2000000000000 N common scale, so large integral `pounds_force` values can overflow `int64_t`. Use a floating `Rep` for mixed-unit force math.
- Conversions to a coarser integral unit truncate toward zero and therefore need an explicit constructor or `quantity_cast`.
