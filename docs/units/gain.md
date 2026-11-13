# Gain

## Overview
Logarithmic gain units for `castle::units`: the bel and the decibel, plus conversions to and from linear power and amplitude ratios. Gain is dimensionless, but it carries the `gain_kind` tag, so it never collapses to a plain number and cannot be mixed with a scalar by accident.

## Header
`#include "castle/units/gain.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `gain_kind` | Kind tag separating gain from plain dimensionless numbers. |
| `bel_unit`, `decibel_unit` | Logarithmic gain units (`bel_unit` is coherent; 1 dB = 0.1 B). |
| `bels<Rep>`, `decibels<Rep>` | `quantity` aliases for the units above. |
| `power_ratio(gain)` | `10^(dB / 10)` for any gain-kind quantity. Returns `Rep` if floating, otherwise `double`. |
| `amplitude_ratio(gain)` | `10^(dB / 20)` for any gain-kind quantity. Returns `Rep` if floating, otherwise `double`. |
| `decibels_from_power_ratio(ratio)` | `decibels<T>` equal to `10 * log10(ratio)`; floating `T` only. |
| `decibels_from_amplitude_ratio(ratio)` | `decibels<T>` equal to `20 * log10(ratio)`; floating `T` only. |
| `literals::_dB`, `_B` | Floating literals give `Rep = double`; integer literals give `Rep = int64_t`. |

All operators, conversions and `quantity_cast` come from [quantity](quantity.md).

## Usage Example
See `samples/units/gain.cpp`.

```cpp
#include "castle/units/gain.hpp"

using namespace castle::units::literals;

const castle::units::decibels<double> total = 3.0_dB + 20.0_dB - 2.0_dB;    // cascade: 21 dB
const double power = castle::units::power_ratio(20.0_dB);                  // 100
const double amplitude = castle::units::amplitude_ratio(20.0_dB);          // 10
const castle::units::decibels<double> measured =
    castle::units::decibels_from_amplitude_ratio(10.0);                    // 20 dB
const castle::units::decibels<int32_t> twenty = castle::units::bels<int32_t>(2); // exact: 20 dB
```

## Constraints & Notes
- Header-only, no allocation, no exceptions; each quantity is exactly `sizeof(Rep)`.
- Adding or subtracting decibels cascades gains (amplifier gain minus cable loss); the linear ratios multiply. Multiplying a gain by a scalar scales the decibel value, it does not scale the linear ratio.
- A gain is built explicitly from a number (`decibels<double>(3.0)`); a plain `double` does not convert to it, and a gain does not convert to a plain number. Use `power_ratio` or `amplitude_ratio` to leave the logarithmic domain.
- `decibels_from_power_ratio` and `decibels_from_amplitude_ratio` need a strictly positive input; the logarithm of zero or a negative value is not checked and yields the C library result (`-inf` or NaN).
- Bels to decibels is exact for integral values; decibels to bels truncates and needs an explicit constructor or `quantity_cast`.
- `pow` and `log10` come from the platform C math library through overloaded `detail` wrappers for `float`, `double` and `long double`.
- Absolute levels are not gain: dBm lives in [power](power.md) (`decibel_milliwatts`), and a level plus a gain is a level there. dBV and dBW are not modelled.
