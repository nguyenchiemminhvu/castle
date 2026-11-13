# Resistance

## Overview
Electric resistance units for `castle::units`: SI multiples of the ohm. Together with [voltage](voltage.md) and [current](current.md), Ohm's law is checked at compile time: `volts / amperes` is assignable to `ohms`, `ohms * amperes` to `volts`, and `volts / ohms` to `amperes`.

## Header
`#include "castle/units/resistance.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dim::resistance` | `dimension<2, 1, -3, -2, 0, 0, 0, 0>` (M L^2 T^-3 I^-2). |
| `ohm_unit`, `milliohm_unit`, `kiloohm_unit`, `megaohm_unit` | SI resistance units (`ohm_unit` is coherent). |
| `ohms<Rep>`, `milliohms<Rep>`, `kiloohms<Rep>`, `megaohms<Rep>` | `quantity` aliases for the resistance units. |
| `literals::_Ohm`, `_mOhm`, `_kOhm`, `_MOhm` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/resistance.cpp`.

```cpp
#include "castle/units/current.hpp"
#include "castle/units/resistance.hpp"
#include "castle/units/voltage.hpp"

using namespace castle::units::literals;

castle::units::amperes<double> current(castle::units::volts<double> shunt_drop, castle::units::ohms<double> shunt)
{
    return shunt_drop / shunt;
}

const castle::units::amperes<double> load = current(25.0_mV, 10.0_mOhm); // 2.5 A
const castle::units::ohms<double> pullup = 3.3_V / 0.33_mA;              // 10000 Ohm
const castle::units::volts<double> drop = 4.7_kOhm * 2.0_mA;             // 9.4 V
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Conversions to a coarser integral unit (for example ohms to kiloohms) truncate toward zero and therefore need an explicit constructor or `quantity_cast`.
- The header does not include voltage.hpp or current.hpp; include them yourself to use Ohm's law.
