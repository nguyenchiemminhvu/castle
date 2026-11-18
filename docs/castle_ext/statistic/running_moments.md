# running_moments

## Overview
Numerically stable one-pass mean, variance, standard deviation, RMS, skewness, and excess kurtosis. `central_moments<R, Order>` is the arithmetic core: it keeps the count, the mean, and the central moment sums M2..M_Order, using the pairwise formulas of Chan, Golub & LeVeque and Pébay (Welford's method is the single-sample case). One update routine adds a sample, retracts a sample, or merges two partitions. `running_moments<T, Order>` wraps the core for unbounded streams and batch ranges: it rejects NaN and infinity and tracks the minimum and maximum. For GNSS work, use it to summarize receiver-reported accuracy or measured fix error; the numbers describe the data you fed in.

## Header
`#include "castle_ext/statistic/running_moments.hpp"`

## Dependencies
- [compiler](../../castle/core/compiler.md)
- [status](../../castle/core/status.md)
- [traits](../../castle/core/traits.md)
- [types](../../castle/core/types.md)
- [algorithm](../../castle/dsa/algorithm.md)
- [array](../../castle/dsa/array.md)
- [sqrt_real](../../castle/math/sqrt_real.md)
- [validation](validation.md)

## Public API
`Order` is the highest maintained moment: 2 (mean, variance), 3 (adds skewness), 4 (adds excess kurtosis). Unused moments cost no storage or arithmetic. Every operation below is O(1) time and O(1) stack unless noted. Output parameters are unchanged on failure.

### `central_moments<R, Order = 2>` (`R` floating-point)
| API | Description / statuses |
| --- | --- |
| `push(value)` | Adds a sample (no validation). `ok`, or `full` when the count equals `SIZE_MAX`. |
| `remove(value)` | Retracts an earlier sample. `ok`, or `empty` with no samples; removing the only sample clears the accumulator. |
| `merge(other)` | Merges another accumulator (may be `*this`). `ok`, or `full` when the combined count would overflow. |
| `clear()` | Removes every sample. |
| `count()`, `empty()` | State queries. |
| `mean(out)` | `ok` or `empty`. |
| `sum()`, `sum_squares()` | Return `mean * n` and `M2 + n * mean^2`; zero when empty. |
| `rms(out)` | `sqrt(sum_squares / n)`. `ok` or `empty`. |
| `population_variance(out)`, `population_stddev(out)` | `M2 / n` and its square root. `ok` or `empty`. |
| `sample_variance(out)`, `sample_stddev(out)` | `M2 / (n - 1)` and its square root. `ok`, or `empty` with fewer than two samples. |
| `skewness(out)` (`Order >= 3`) | `g1 = sqrt(n) * M3 / M2^1.5`. `ok`, `empty`, or `invalid_argument` when the variance is zero. |
| `excess_kurtosis(out)` (`Order >= 4`) | `g2 = n * M4 / M2^2 - 3`. `ok`, `empty`, or `invalid_argument` when the variance is zero. |
| `order` | Static constant: `Order`. |

### `running_moments<T, Order = 2>` (`T` arithmetic; results in `result_t<T>`)
| API | Description / statuses |
| --- | --- |
| `push(sample)` | `ok`, `invalid_argument` for NaN/infinity (floating `T` only), or `full`. Updates minimum and maximum. |
| `push_range(first, last)` | Pushes each element. O(N) time. Returns `ok` or the first failing status; earlier samples stay added. |
| `merge(other)` | Merges moments and extremes. `ok` or `full` (nothing changes on `full`). |
| `minimum(out)`, `maximum(out)`, `range(out)` | `ok` or `empty`. |
| `clear()` | Removes every sample and resets the extremes. |
| `count`, `empty`, `mean`, `sum`, `sum_squares`, `rms`, `population_variance`, `sample_variance`, `population_stddev`, `sample_stddev`, `skewness`, `excess_kurtosis` | Inherited from `central_moments` with the same semantics. |

`remove()` is intentionally not offered on `running_moments`: it would invalidate the tracked extremes. Use [sliding_moments](sliding_moments.md) for windows.

## Usage Example
See [`samples/castle_ext/statistic/running_moments.cpp`](../../../samples/castle_ext/statistic/running_moments.cpp).

```cpp
castle::statistic::running_moments<double, 4U> stats;
const double batch[] = {1.0, 2.0, 3.0, 4.0, 10.0};
stats.push_range(batch, batch + 5);
double variance = 0.0;
if (stats.sample_variance(variance) == castle::status::ok)
{
    // variance == 12.5
}
```

## Constraints & Notes
- No allocation, no exceptions. Storage: one `size_type`, one `R` mean, `Order - 1` moment sums (plus two `R` extremes in `running_moments`).
- `central_moments` does not validate input; use `running_moments` or `sliding_moments` for NaN/infinity rejection.
- `remove()` cancels digits; long add/remove sequences accumulate rounding error. `sliding_moments` bounds that drift.
- M2 is clamped at zero when computing variance, RMS, and standard deviation, so a tiny negative rounding residue never yields NaN.
- `skewness()` and `excess_kurtosis()` have a `static_assert` on `Order`; calling them on a lower order does not compile.
- Reference values for `{1, 2, 3, 4, 10}`: mean 4, M2 50, M3 180, M4 1394, population variance 10, sample variance 12.5, skewness 1.13842, excess kurtosis -0.212.
- Not thread-safe; merge per-thread accumulators instead of sharing one.
