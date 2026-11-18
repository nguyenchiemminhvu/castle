# KernelDensity

## Overview
Kernel density estimation (KDE) turns a set of samples into a smooth density curve: `f(x) = 1 / (n * h) * sum K((x - x_i) / h)`. `kernel_density()` evaluates that estimate at one point over any input range (a caller-owned array, a `sample_window` or one of its `latest()` views, or `sorted_window::sorted()`). The kernel `K` is a template parameter, so only the chosen kernel is compiled in. `normal_reference_bandwidth()` supplies the smoothing bandwidth `h` from the data.

## Header
`#include "castle_ext/statistic/kernel_density.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../castle/core/compiler.md)
- [`castle/core/constants.hpp`](../../castle/core/constants.md)
- [`castle/core/status.hpp`](../../castle/core/status.md)
- [`castle/core/traits.hpp`](../../castle/core/traits.md)
- [`castle/core/types.hpp`](../../castle/core/types.md)
- [`castle/math/abs.hpp`](../../castle/math/abs.md)
- [`castle/math/exponential.hpp`](../../castle/math/exponential.md)
- [`castle/math/sqrt_real.hpp`](../../castle/math/sqrt_real.md)
- [`castle_ext/statistic/validation.hpp`](validation.md)
- [`castle_ext/statistic/running_moments.hpp`](running_moments.md)

## Public API
All functions are `noexcept`. `R` is a floating-point type deduced from the `out` argument; `x`, `bandwidth` are converted to `R`.

### Kernels
Each kernel is a struct with `template <typename R> static R evaluate(R u)`; all are O(1) time and stack.

|Kernel|Formula|Support|
|---|---|---|
|`uniform_kernel`|`1/2`|`\|u\| <= 1` (boundary included)|
|`triangular_kernel`|`1 - \|u\|`|`\|u\| <= 1`|
|`epanechnikov_kernel`|`3/4 * (1 - u^2)`|`\|u\| <= 1`|
|`gaussian_kernel`|`exp(-u^2 / 2) / sqrt(2 * pi)`|unbounded (uses `castle::math::exp`)|

Outside the support the compact kernels return 0.

### Functions
|Signature|Description|Status returns|Time / stack|
|---|---|---|---|
|`template <typename TKernel = gaussian_kernel, typename TIterator, typename R> status kernel_density(TIterator first, TIterator last, R x, R bandwidth, R& out)`|Density estimate at `x`.|`ok`, `invalid_argument` (non-finite `x`, or `bandwidth` not finite and positive), `empty` (empty range)|O(N) / O(1)|
|`template <typename TIterator, typename R> status normal_reference_bandwidth(TIterator first, TIterator last, R& out)`|Silverman rule `h = 1.06 * s * n^(-1/5)` with `s` the sample standard deviation.|`ok`, `empty` (fewer than 2 samples), `invalid_argument` (non-finite sample or zero spread)|O(N) / O(1)|

`out` is unchanged on every failure.

## Usage Example
```cpp
#include "castle_ext/statistic/kernel_density.hpp"

const double samples[] = {1.0, 1.5, 2.0, 4.0};
double h = 0.0;
double density = 0.0;
if (castle::statistic::normal_reference_bandwidth(samples, samples + 4, h) == castle::status::ok &&
    castle::statistic::kernel_density<castle::statistic::epanechnikov_kernel>(samples, samples + 4, 1.5, h, density) ==
        castle::status::ok)
{
    // density estimate at x = 1.5
}
```

See [`samples/castle_ext/statistic/kernel_density.cpp`](../../../samples/castle_ext/statistic/kernel_density.cpp).

## Constraints & Notes
- No allocation; each query is a single pass, O(N) time and O(1) stack. Evaluating a whole curve over M points costs O(M * N).
- The range elements must be finite; `kernel_density()` does not check them. `normal_reference_bandwidth()` does, through `running_moments::push_range`.
- The bandwidth matters more than the kernel. The rule of thumb is near-optimal for unimodal, roughly normal data and over-smooths multimodal data.
- Only `gaussian_kernel` and `normal_reference_bandwidth()` use `<math.h>` (`exp`, `log`); the compact kernels are pure arithmetic. A `float` result type keeps the computation in single precision.
- Not thread-safe for shared mutable ranges; the functions themselves hold no state.
- GNSS note: a density over recent hAcc values shows whether the receiver's reported accuracy is a single cluster or has outliers; for percentiles use [`cep.md`](cep.md) instead.
