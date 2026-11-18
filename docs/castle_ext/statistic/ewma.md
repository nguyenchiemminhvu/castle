# Ewma

## Overview
`castle::statistic::ewma<T>` is an exponentially weighted moving average (a first-order IIR smoother) that also tracks an exponentially weighted variance. Each new sample moves the mean toward it by the fraction `alpha`, so older samples fade by a factor of `(1 - alpha)` per step. A larger `alpha` reacts faster; a smaller `alpha` smooths more. `alpha = 2 / (N + 1)` gives the same centre of mass as an N-sample moving average. No sample is stored, so the filter costs a few values of memory no matter how long the stream runs. Typical uses are smoothing a speed, temperature, or receiver-reported accuracy (`hAcc`) value before display or thresholding.

## Header
`#include "castle_ext/statistic/ewma.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../castle/core/compiler.md)
- [`castle/core/status.hpp`](../../castle/core/status.md)
- [`castle/core/traits.hpp`](../../castle/core/traits.md)
- [`castle/math/sqrt_real.hpp`](../../castle/math/sqrt_real.md)
- [`castle_ext/statistic/validation.hpp`](validation.md)

## Public API

### `template <typename T = double> class ewma`

`T` must be an arithmetic type. `result_type` is `result_t<T>`: `T` for floating-point samples and `double` for integral samples.

| Member | Signature | Description | Returns | Time | Stack |
|---|---|---|---|---|---|
| Default constructor | `ewma()` | Creates an unconfigured filter. | — | O(1) | O(1) |
| Constructor | `explicit ewma(result_type alpha)` | Creates a filter and calls `configure(alpha)`. An invalid `alpha` leaves it unconfigured. | — | O(1) | O(1) |
| `configure` | `status configure(result_type alpha)` | Sets the smoothing factor and discards the state. `alpha` must be in (0, 1]. | `ok`, `invalid_argument` (the filter becomes unconfigured and `alpha()` is 0) | O(1) | O(1) |
| `configured` | `bool configured() const` | Reports whether a valid `alpha` is set. | `bool` | O(1) | O(1) |
| `alpha` | `result_type alpha() const` | Current smoothing factor. 0 when unconfigured. | value | O(1) | O(1) |
| `empty` | `bool empty() const` | `true` until the first sample after configuration or `clear()`. | `bool` | O(1) | O(1) |
| `push` | `status push(T sample)` | Feeds one sample. The first sample sets the mean and makes the variance 0. Later samples apply `diff = x - mean`, `mean += alpha * diff`, `var = (1 - alpha) * (var + diff * alpha * diff)`. | `ok`, `not_configured`, `invalid_argument` (NaN or infinity) | O(1) | O(1) |
| `mean` | `status mean(result_type& out) const` | Smoothed value. | `ok`, `empty` | O(1) | O(1) |
| `variance` | `status variance(result_type& out) const` | Exponentially weighted variance. | `ok`, `empty` | O(1) | O(1) |
| `stddev` | `status stddev(result_type& out) const` | Square root of the variance. | `ok`, `empty` | O(1) (one `sqrt_real`) | O(1) |
| `clear` | `void clear()` | Discards the state but keeps `alpha`. | — | O(1) | O(1) |

When a call fails, `out` is left unchanged.

## Usage Example
```cpp
#include "castle_ext/statistic/ewma.hpp"

castle::statistic::ewma<double> filter(0.5);
filter.push(10.0); // mean 10, variance 0
filter.push(20.0); // mean 15, variance 25
double mean = 0.0;
double sigma = 0.0;
if ((filter.mean(mean) == castle::status::ok) && (filter.stddev(sigma) == castle::status::ok))
{
    // mean == 15.0, sigma == 5.0
}
```

See [`samples/castle_ext/statistic/ewma.cpp`](../../../samples/castle_ext/statistic/ewma.cpp) for a complete program.

## Constraints & Notes
- Storage is three `result_type` values plus two flags. There is no heap allocation and no sample history.
- Each `push()` and query takes O(1) time and O(1) stack.
- `alpha` must be in (0, 1]. `alpha = 1` tracks the input exactly and keeps the variance at 0. Values of 0 or below, above 1, and NaN are rejected.
- The variance uses the incremental form from Finch (2009). With `0 < alpha <= 1` it stays non-negative. It is an exponentially weighted estimate, not the unbiased sample variance of a fixed window.
- Floating-point samples that are NaN or infinite are rejected with `status::invalid_argument` and leave the state unchanged. Integral samples are always accepted and are converted to `double`.
- Calling `configure()` always clears the state, even when it fails.
- The class is not thread-safe. Protect shared instances externally.
