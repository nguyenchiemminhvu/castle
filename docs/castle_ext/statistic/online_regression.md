# OnlineRegression

## Overview
`online_regression<T>` keeps the running means of two variables x and y plus their co-moment sums, so it can report covariance, the Pearson correlation coefficient, and the least-squares line `y = intercept + slope * x` after any number of observations, without storing them. It uses Welford-style updates, which avoid the cancellation of the textbook `sum(xy) - sum(x) * sum(y) / n` formula. Typical uses are receiver clock drift (offset versus time), sensor calibration, and trend detection on a stream.

## Header
`#include "castle_ext/statistic/online_regression.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../castle/core/compiler.md)
- [`castle/core/status.hpp`](../../castle/core/status.md)
- [`castle/core/traits.hpp`](../../castle/core/traits.md)
- [`castle/core/types.hpp`](../../castle/core/types.md)
- [`castle/math/clamp.hpp`](../../castle/math/clamp.md)
- [`castle/math/sqrt_real.hpp`](../../castle/math/sqrt_real.md)
- [`castle_ext/statistic/validation.hpp`](validation.md)

## Public API
`result_type` is `result_t<T>`: `T` for floating-point samples, `double` for integral samples. All members are `noexcept`.

### `castle::statistic::online_regression<T = double>`
|Signature|Description|Status returns|Time / stack|
|---|---|---|---|
|`online_regression()`|Constructs an empty model.|-|O(1) / O(1)|
|`status push(T x, T y)`|Adds one observation. Floating-point `T` rejects NaN and infinity in either value.|`ok`, `invalid_argument`, `full` (counter exhausted)|O(1) / O(1)|
|`size_type count() const`|Number of observations.|-|O(1) / O(1)|
|`bool empty() const`|`true` when no observation was added.|-|O(1) / O(1)|
|`status mean_x(result_type& out) const`|Mean of x.|`ok`, `empty` (needs 1)|O(1) / O(1)|
|`status mean_y(result_type& out) const`|Mean of y.|`ok`, `empty` (needs 1)|O(1) / O(1)|
|`status population_covariance(result_type& out) const`|`Sxy / n`.|`ok`, `empty` (needs 1)|O(1) / O(1)|
|`status sample_covariance(result_type& out) const`|`Sxy / (n - 1)`.|`ok`, `empty` (needs 2)|O(1) / O(1)|
|`status correlation(result_type& out) const`|Pearson `r = Sxy / sqrt(Sxx * Syy)`, clamped to [-1, 1].|`ok`, `empty` (needs 2), `invalid_argument` (x or y has zero variance)|O(1) / O(1)|
|`status fit(result_type& slope, result_type& intercept) const`|Least-squares line: `slope = Sxy / Sxx`, `intercept = mean_y - slope * mean_x`.|`ok`, `empty` (needs 1), `invalid_argument` (every x identical)|O(1) / O(1)|
|`void clear()`|Discards every observation.|-|O(1) / O(1)|

Outputs are left unchanged whenever a status other than `ok` is returned.

## Usage Example
```cpp
#include "castle_ext/statistic/online_regression.hpp"

castle::statistic::online_regression<double> drift;
drift.push(0.0, 1.0); // time [s], clock offset [us]
drift.push(1.0, 3.0);
drift.push(2.0, 5.0);

double slope = 0.0;
double intercept = 0.0;
if (drift.fit(slope, intercept) == castle::status::ok)
{
    // slope == 2.0 us/s, intercept == 1.0 us
}
```

See [`samples/castle_ext/statistic/online_regression.cpp`](../../../samples/castle_ext/statistic/online_regression.cpp).

## Constraints & Notes
- Storage: one counter and five `result_type` values; no heap allocation. Every operation is O(1) time and stack.
- Not thread-safe; protect shared instances externally.
- `correlation()` and `fit()` need variance in x; `correlation()` also needs variance in y. A constant y still gives a valid flat fit (slope 0) but no correlation.
- The correlation is clamped so rounding can never push a perfectly linear stream outside [-1, 1].
- Floating-point accumulation is in `result_type`; with `float` samples expect single-precision rounding in the sums.
- The counter saturation check (`status::full`) needs `SIZE_MAX` observations and is not practically reachable.
- GNSS note: regressing clock offset on time yields drift; regressing a reported hAcc against the observed error shows how well the receiver's accuracy estimate tracks reality.
