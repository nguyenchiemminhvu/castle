# Frequency

## Overview
Frequency units for `castle::units`: the hertz and its SI multiples. Use them in place of raw `double` rates so that a function taking `hertz<double>` cannot receive a period, an angular velocity, or a value in the wrong scale.

## Header
`#include "castle/units/frequency.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `dim::frequency` | Dimension `T^-1` with no angle exponent (cycles per time). |
| `hertz_unit`, `millihertz_unit`, `kilohertz_unit`, `megahertz_unit`, `gigahertz_unit` | SI frequency units (`hertz_unit` is coherent). |
| `hertz<Rep>`, `millihertz<Rep>`, `kilohertz<Rep>`, `megahertz<Rep>`, `gigahertz<Rep>` | `quantity` aliases for the units. |
| `literals::_Hz`, `_mHz`, `_kHz`, `_MHz`, `_GHz` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions, `inverse` and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/frequency.cpp`.

```cpp
#include "castle/units/frequency.hpp"
#include "castle/units/time.hpp"

using namespace castle::units::literals;

castle::units::seconds<double> period = 1.0 / castle::units::hertz<double>(2.0_kHz); // 0.0005 s
const castle::units::hertz<double> rate = castle::units::inverse(20.0_ms);           // 50 Hz
const double cycles = 50.0_Hz * 2.0_s;                                               // 100 cycles
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Frequency is not angular velocity: `revolutions_per_minute` does not convert to `hertz` (see [angular_velocity](angular_velocity.md)).
- `1.0 / frequency` and `inverse(duration)` follow the reciprocal scale: `inverse(20.0_ms)` is a `kilohertz<double>` of 0.05, which converts implicitly to 50 Hz.
- `1.0 / frequency` is a `castle::chrono::duration` in seconds; frequency times a duration is a plain cycle count.
- Conversions to a coarser integral unit (for example millihertz to hertz) truncate toward zero and need an explicit constructor or `quantity_cast`.
