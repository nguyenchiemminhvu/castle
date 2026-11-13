# Units

## Overview
Umbrella header for `castle::units`, CASTLE's type-safe physical quantities. Write `set_velocity(meters_per_second<double>)` instead of `set_velocity(double)`: the compiler rejects an angle or a frequency, converts km/h or knots exactly, and derives units through arithmetic (`meters / seconds` gives `meters_per_second`). This module is also the official home of CASTLE's SI prefixes, time scales and unit literals.

## Header
`#include "castle/units/units.hpp"`

## Dependencies
- [scale](scale.md): SI prefixes and time scales
- [quantity](quantity.md): the engine (dimensions, units, conversions, operators)
- [time](time.md): `chrono::duration` aliases and time literals
- [angle](angle.md), [geo](geo.md), [distance](distance.md), [velocity](velocity.md), [acceleration](acceleration.md), [angular_velocity](angular_velocity.md), [frequency](frequency.md)
- [temperature](temperature.md), [pressure](pressure.md), [mass](mass.md), [force](force.md), [energy](energy.md), [power](power.md)
- [voltage](voltage.md), [current](current.md), [resistance](resistance.md), [charge](charge.md), [gain](gain.md)

## Public API
| API | Description |
|---|---|
| Everything from the headers above | No additional declarations. |
| `castle::units::literals` | One namespace for every unit literal (`_m`, `_kmph`, `_deg`, `_ms`, `_degC`, `_dB`, ...). |

## Usage Example
See `samples/units/units.cpp`.

```cpp
#include "castle/units/units.hpp"

using namespace castle::units::literals;

void set_velocity(castle::units::meters_per_second<double> speed);

set_velocity(36.0_kmph);        // 10 m/s
set_velocity(100.0_m / 4_s);    // 25 m/s
// set_velocity(5.0_deg);       // compile error
```

## Constraints & Notes
- Header-only. No heap, exceptions, RTTI, virtual functions or STL. Each quantity is exactly as large as its representation.
- Including the umbrella pulls in `<math.h>` (angle trigonometry, gain and dBm logarithms). Include individual headers on very small targets.
- Literal suffixes are unique across the module, so `using namespace castle::units::literals;` never causes ambiguity.
