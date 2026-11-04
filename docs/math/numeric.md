# Numeric Methods Umbrella

## Overview
Top-level Castle numerical-methods umbrella header. Include it when a translation unit needs numerical integration, differentiation, root finding and ODE stepping from one entry point. Every routine is header-only, allocation-free, exception-free and non-recursive, takes callables as templates (no `std::function`) and reports iterative outcomes through `castle::status` / `numeric_result`.

## Header
`#include "castle/math/numeric.hpp"`

## Dependencies
- [`castle/math/numeric/result.hpp`](numeric/result.md)
- [`castle/math/numeric/integration.hpp`](numeric/integration.md)
- [`castle/math/numeric/differentiation.hpp`](numeric/differentiation.md)
- [`castle/math/numeric/root_finding.hpp`](numeric/root_finding.md)
- [`castle/math/numeric/ode.hpp`](numeric/ode.md)

## Public API
| API | Description |
| --- | --- |
| `#include "castle/math/numeric.hpp"` | Re-exports `numeric_result`, the integration routines (`integrate_trapezoid`, `integrate_simpson`, `integrate_gauss_legendre`, `integrate_romberg` and the sampled-data variants), the differentiation routines (`derivative_*`), the root finders (`find_root_bisection`, `find_root_newton`, `find_root_secant`) and the ODE steppers (`euler_step`, `rk4_step`). |

## Usage Example
```cpp
#include "castle/math/numeric.hpp"

const auto f = [](double x) { return x * x; };
const double area = castle::math::integrate_simpson(f, 0.0, 3.0, 100U);   // 9
const double slope = castle::math::derivative_central(f, 3.0, 1.0e-4);    // 6
const auto g = [](double x) { return x * x - 2.0; };
const auto root = castle::math::find_root_bisection(g, 0.0, 2.0, 1.0e-9); // sqrt(2)
(void)area;
(void)slope;
(void)root;
```

See [`samples/math/numeric.cpp`](../../samples/math/numeric.cpp).

## Constraints & Notes
- This header adds no new runtime logic; it only includes the individual numeric headers.
- Prefer the narrower headers when compile-time footprint matters.
- Callables must be non-throwing; iterative routines are bounded by caller-supplied iteration limits for deterministic worst-case execution time.
