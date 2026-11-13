# Angle

## Overview
Plane-angle units, unit-safe trigonometry and angle wrapping. Angle is its own dimension in `castle::units`, so a heading in degrees cannot be passed where a raw scalar, a frequency, or a geographic latitude is expected. Units that are a fraction of pi (degrees, arc-minutes, arc-seconds, revolutions) record an exact pi factor relative to radians.

## Header
`#include "castle/units/angle.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [constants](../core/constants.md)
- [traits](../core/traits.md)
- [type_ranges](../core/type_ranges.md)
- [trigonometry](../math/linalg/trigonometry.md)
- [mod](../math/mod.md)
- [quantity](quantity.md)
- [scale](scale.md)

## Public API
| API | Description |
|---|---|
| `radian_unit`, `milliradian_unit`, `degree_unit`, `arcminute_unit`, `arcsecond_unit`, `revolution_unit` | Angle units. `radian_unit` is coherent; the others from degrees on carry a pi factor. |
| `radians<Rep>`, `milliradians<Rep>`, `degrees<Rep>`, `arcminutes<Rep>`, `arcseconds<Rep>`, `revolutions<Rep>` | `quantity` aliases. |
| `sin(a)`, `cos(a)`, `tan(a)` | Take any angle quantity of any kind and return the floating type of `Rep` (`double` for integral reps). Found by ADL. |
| `asin(x)`, `acos(x)`, `atan2(y, x)` | Floating inputs, `radians<T>` result. |
| `atan2(qy, qx)` | Two quantities of the same dimension and kind (e.g. east/north metres); returns `radians<real>`. |
| `wrap_positive(a)` | Wraps into `[0, one turn)` in the angle's own unit and kind (e.g. `[0, 360)` degrees). |
| `wrap_signed(a)` | Wraps into `[-half turn, half turn)` (e.g. `[-180, 180)` degrees). |
| `literals::_rad`, `_mrad`, `_deg`, `_arcmin`, `_arcsec`, `_rev` | Floating literals give `double`; integer literals give `int64_t`. |

## Usage Example
See `samples/units/angle.cpp`.

```cpp
#include "castle/units/angle.hpp"

using namespace castle::units::literals;

castle::units::degrees<double> heading = castle::units::wrap_positive(370.0_deg); // 10 deg
const double s = castle::units::sin(heading);
const castle::units::degrees<double> error = castle::units::wrap_signed(350.0_deg - 10.0_deg); // -20 deg

// Fixed-point field: 0.01 degree steps in int32_t.
using centidegree_unit = castle::units::scaled_unit_t<castle::units::degree_unit, castle::units::centi>;
castle::units::quantity<centidegree_unit, int32_t> raw(-9000);
raw = castle::units::wrap_positive(raw); // 27000
```

## Constraints & Notes
- Trigonometry calls the platform C math library through `castle/math/linalg/trigonometry.hpp`. `tan` is computed as `sin / cos`, and `acos` as `pi/2 - asin`.
- Floating wrapping uses `fmod`. A tiny negative input that rounds to exactly one turn is folded back to zero, so the upper bound stays open.
- Integral wrapping needs a pi factor and a whole number of units per turn (degrees, centidegrees, arc-minutes, ...), and the turn must fit `Rep`. This is checked at compile time. `wrap_signed` also needs a signed `Rep`.
- Converting between units with different pi factors (degrees and radians) runs in floating point; an integral destination truncates and needs an explicit conversion.
- Angle is a dimension, so `radius * angle` is `length·angle`, not length. Use `.value()` for arc-length formulas.
