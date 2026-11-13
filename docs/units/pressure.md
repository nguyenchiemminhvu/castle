# Pressure

## Overview
Pressure units for `castle::units`: SI multiples of the pascal plus bar, millibar, standard atmosphere and psi. Use them in place of raw numbers so that a barometer or tyre-pressure API cannot mix hectopascals, psi and pascals.

## Header
`#include "castle/units/pressure.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dim::pressure` | `dimension<-1, 1, -2, 0, 0, 0, 0, 0>` (M L^-1 T^-2). |
| `pascal_unit`, `hectopascal_unit`, `kilopascal_unit`, `megapascal_unit` | SI pressure units (`pascal_unit` is coherent). |
| `bar_unit`, `millibar_unit`, `atmosphere_unit`, `pound_per_square_inch_unit` | Exact definitions: 100000 Pa, 100 Pa, 101325 Pa, 44482216152605 / 6451600000 Pa. |
| `pascals<Rep>`, `hectopascals<Rep>`, `kilopascals<Rep>`, `megapascals<Rep>` | `quantity` aliases for the SI units. |
| `bars<Rep>`, `millibars<Rep>`, `atmospheres<Rep>`, `pounds_per_square_inch<Rep>` | `quantity` aliases for the non-SI units. |
| `literals::_Pa`, `_hPa`, `_kPa`, `_MPa`, `_bar`, `_mbar`, `_atm`, `_psi` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/pressure.cpp`.

```cpp
#include "castle/units/pressure.hpp"

using namespace castle::units::literals;

bool tyre_underinflated(castle::units::kilopascals<double> measured)
{
    return measured < castle::units::kilopascals<double>(200.0);
}

const castle::units::hectopascals<double> qnh = 1_atm;          // 1013.25 hPa
const bool low = tyre_underinflated(28.0_psi);                   // true (about 193 kPa)
const castle::units::bars<int64_t> coarse(2_bar + 150_hPa);      // explicit: 2 bar
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Conversions to a coarser integral unit (for example pascals to bars) truncate toward zero and therefore need an explicit constructor or `quantity_cast`.
- `hectopascal_unit` and `millibar_unit` are the same type, so `1_hPa == 1_mbar` exactly.
- psi is not an integral multiple of the pascal, so integral psi values convert to other units only explicitly; prefer a floating `Rep`.
