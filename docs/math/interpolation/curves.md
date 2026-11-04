# Interpolation Curves

## Overview
Stateless, allocation-free interpolation kernels for floating-point values: bilinear interpolation inside a unit cell, cubic Hermite segments (with derivative), uniform Catmull-Rom segments and cubic Bezier curves. They are the closed-form building blocks of the table-based interpolators ([`cubic_spline`](cubic_spline.md)) and are equally useful on their own for animation, path smoothing and calibration maps.

## Header
`#include "castle/math/interpolation/curves.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../core/compiler.md)
- [`castle/core/traits.hpp`](../../core/traits.md)
- [`castle/math/interpolation/lerp.hpp`](lerp.md)

## Public API
All functions are `constexpr`, `noexcept`, `[[nodiscard]]` templates that participate only for floating-point `T`. Every call is `O(1)`.

| Signature | Description |
| --- | --- |
| `template <typename T> T bilinear(T f00, T f10, T f01, T f11, T tx, T ty) noexcept` | Bilinear interpolation in a unit cell. `f00`/`f10`/`f01`/`f11` are the corner values at `(0,0)`, `(1,0)`, `(0,1)`, `(1,1)`; `tx`/`ty` are the normalized positions along x and y. Returns `lerp` along x on both rows, then `lerp` along y. |
| `template <typename T> T cubic_hermite(T p0, T m0, T p1, T m1, T t) noexcept` | Cubic Hermite segment. `p0`/`p1` are the end values at `t = 0` / `t = 1`, `m0`/`m1` the end tangents expressed per unit of `t`. Returns the unique cubic matching both values and tangents. For a segment of width `h` in x, pass `h * slope` as tangents. |
| `template <typename T> T cubic_hermite_derivative(T p0, T m0, T p1, T m1, T t) noexcept` | `d/dt` of `cubic_hermite` with the same arguments. Divide by the segment width `h` to obtain the slope with respect to x. |
| `template <typename T> T catmull_rom(T p0, T p1, T p2, T p3, T t) noexcept` | Uniform Catmull-Rom segment between `p1` (`t = 0`) and `p2` (`t = 1`); `p0` and `p3` are the outer control values. Equivalent to `cubic_hermite` with tangents `(p2 - p0) / 2` and `(p3 - p1) / 2`. |
| `template <typename T> T cubic_bezier(T p0, T p1, T p2, T p3, T t) noexcept` | Cubic Bezier curve. `p0`/`p3` are the end control values (reached at `t = 0` / `t = 1`), `p1`/`p2` the handles. Returns the Bernstein-weighted combination of the four values. |

## Usage Example
```cpp
#include "castle/math/interpolation/curves.hpp"

constexpr float mid = castle::math::catmull_rom(0.0F, 1.0F, 2.0F, 3.0F, 0.5F);    // 1.5
constexpr float cell = castle::math::bilinear(0.0F, 1.0F, 2.0F, 3.0F, 0.5F, 0.5F); // 1.5
constexpr float curve = castle::math::cubic_bezier(0.0F, 0.0F, 1.0F, 1.0F, 0.5F);  // 0.5
```

See [`samples/math/interpolation/curves.cpp`](../../../samples/math/interpolation/curves.cpp).

## Constraints & Notes
- `t`, `tx` and `ty` are intentionally not clamped; values outside `[0, 1]` extrapolate the underlying polynomial.
- Only floating-point types are accepted; integral arguments fail template deduction (SFINAE).
- Linear interpolation itself lives in [`castle/math/interpolation/lerp.hpp`](lerp.md).
- Catmull-Rom is uniform: it assumes equally spaced parameter values and does not adapt to uneven knot spacing.
- No overflow or NaN checks are performed; non-finite input yields non-finite output.
