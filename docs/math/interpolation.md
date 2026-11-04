# Interpolation Umbrella

## Overview
Umbrella header for Castle's interpolation facilities. Include it when a translation unit needs the scalar kernels (`lerp`, bilinear, cubic Hermite, Catmull-Rom, cubic Bezier) and the table-based interpolators (`linear_table`, `cubic_spline`) from one entry point. All components are header-only, allocation-free, exception-free and use fixed or caller-owned storage.

## Header
`#include "castle/math/interpolation.hpp"`

## Dependencies
- [`castle/math/interpolation/lerp.hpp`](interpolation/lerp.md)
- [`castle/math/interpolation/knots.hpp`](interpolation/knots.md)
- [`castle/math/interpolation/curves.hpp`](interpolation/curves.md)
- [`castle/math/interpolation/linear_table.hpp`](interpolation/linear_table.md)
- [`castle/math/interpolation/cubic_spline.hpp`](interpolation/cubic_spline.md)

## Public API
| API | Description |
| --- | --- |
| `#include "castle/math/interpolation.hpp"` | Re-exports `lerp`, `linear_interpolate`, the `extrapolation` enum, the curve kernels (`bilinear`, `cubic_hermite`, `cubic_hermite_derivative`, `catmull_rom`, `cubic_bezier`), `linear_table` and `cubic_spline` / `spline_kind`. |

## Usage Example
```cpp
#include "castle/math/interpolation.hpp"

const float xs[] = {0.0F, 1.0F, 2.0F};
const float ys[] = {0.0F, 2.0F, 8.0F};
const castle::math::linear_table<float> table(xs, ys, 3U);
const float y = table.evaluate(1.5F); // 5

castle::math::cubic_spline<float, 4U> spline;
if (spline.build(xs, ys, 3U) == castle::status::ok)
{
    const float smooth = spline.evaluate(1.5F);
}
```

See [`samples/math/interpolation.cpp`](../../samples/math/interpolation.cpp).

## Constraints & Notes
- This header adds no new runtime logic; it only includes the headers listed above.
- It is also pulled in by the full math umbrella `castle/math/math.hpp`.
