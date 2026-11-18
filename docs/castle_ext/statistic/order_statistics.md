# OrderStatistics

## Overview
`order_statistics.hpp` gives exact batch quantiles, the median, the interquartile range (IQR), and the mode for a range of samples that the caller owns. The `*_sorted` functions read a range that is already sorted ascending. `sort_samples()` checks a range and sorts it in place. `quantile_inplace()` does both steps for a one-off query. Nothing allocates memory and there is no hidden workspace, so the functions suit embedded targets that log a batch of measurements and then summarise it. For example, a GNSS test can sort the horizontal error of every fix and read the 50% or 95% CEP radius directly.

Two quantile definitions are available (Hyndman & Fan, 1996):

- `quantile_method::linear` (type 7, the default in R and NumPy) interpolates between the two order statistics around position `p * (n - 1)`.
- `quantile_method::nearest_rank` (type 1) returns the smallest sample whose empirical CDF reaches `p`, so at least `p * n` samples are less than or equal to the result. This matches the "p% of fixes lie within r" wording used in GNSS test reports.

## Header
`#include "castle_ext/statistic/order_statistics.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../castle/core/compiler.md)
- [`castle/core/status.hpp`](../../castle/core/status.md)
- [`castle/core/traits.hpp`](../../castle/core/traits.md)
- [`castle/core/types.hpp`](../../castle/core/types.md)
- [`castle/dsa/algorithm.hpp`](../../castle/dsa/algorithm.md)
- [`castle_ext/statistic/validation.hpp`](validation.md)

## Public API

### `enum class quantile_method : uint8_t`

| Enumerator | Description |
|---|---|
| `linear` | Hyndman–Fan type 7: linear interpolation at `p * (n - 1)`. This is the default. |
| `nearest_rank` | Hyndman–Fan type 1: the sample at zero-based index `ceil(p * n) - 1`, clamped to `[0, n - 1]`. |

### Functions

In every query function, `R` is a floating-point result type deduced from `out`. On failure, `out` is left unchanged.

| Function | Description | Returns | Time | Stack |
|---|---|---|---|---|
| `status quantile_sorted(first, last, R probability, R& out, quantile_method method = linear)` | Quantile of an ascending, finite range. | `ok`; `invalid_argument` when `probability` is outside [0, 1] or NaN; `empty` for an empty range | O(1) | O(1) |
| `status median_sorted(first, last, R& out)` | Linear quantile at 0.5. For an even count, this is the mean of the two middle samples. | `ok`, `empty` | O(1) | O(1) |
| `status interquartile_range_sorted(first, last, R& out, quantile_method method = linear)` | `Q3 - Q1`, using the chosen method. | `ok`, `empty` | O(1) | O(1) |
| `status mode_sorted(first, last, R& out)` | Most frequent value, found by exact equality. Ties go to the smallest value. Needs only a forward iterator. | `ok`, `empty` | O(N) | O(1) |
| `status sort_samples(first, last)` | Checks every sample and sorts the range ascending in place with `castle::sort` (heapsort). Only floating-point samples are checked; integral samples are always valid. | `ok`; `invalid_argument` if any sample is NaN or infinite (the range is not modified) | O(N log N) worst case | O(1) |
| `status quantile_inplace(first, last, R probability, R& out, quantile_method method = linear)` | Checks `probability`, then calls `sort_samples()`, then `quantile_sorted()`. The range ends up sorted on success. | `ok`; `invalid_argument` for a bad probability (checked before sorting) or a non-finite sample; `empty` | O(N log N) | O(1) |

`detail::nearest_rank_index(p, n)` is an internal helper. It computes `ceil(p * n) - 1` with a relative tolerance of 16 epsilon, so a product such as `0.95 * 100` that rounds to `95.00000000000001` still selects index 94 instead of 95.

## Usage Example
```cpp
#include "castle_ext/statistic/order_statistics.hpp"

double errors[] = {2.0, 0.5, 1.5, 1.0};   // horizontal errors in metres
if (castle::statistic::sort_samples(errors, errors + 4) == castle::status::ok)
{
    double median = 0.0;
    double p95 = 0.0;
    (void)castle::statistic::median_sorted(errors, errors + 4, median);       // 1.25
    (void)castle::statistic::quantile_sorted(errors, errors + 4, 0.95, p95);  // 1.925
}
```

The full program is in [`samples/castle_ext/statistic/order_statistics.cpp`](../../../samples/castle_ext/statistic/order_statistics.cpp).

## Constraints & Notes
- The `*_sorted` functions do not check their input. The range must be sorted ascending and contain only finite values. Call `sort_samples()` first if you are not sure.
- To compute several statistics, sort once with `sort_samples()` and then use the O(1) `*_sorted` queries. Avoid calling `quantile_inplace()` repeatedly.
- No heap allocation and no recursion: sorting uses heapsort, which takes O(N log N) time in the worst case and O(1) stack.
- `quantile_inplace()` and `sort_samples()` change the order of the caller's range.
- Integral samples are accepted. Results are converted to the floating-point type `R`.
- `mode_sorted()` uses exact equality. That is useful for discrete data such as integers or quantised readings. For continuous data, use [`histogram`](histogram.md).
- For GNSS data, a percentile of the true horizontal error is a CEP radius. A percentile of the receiver-reported `hAcc` describes the receiver's accuracy estimate, not the true error.
- The functions are pure apart from writing to the caller's range and `out`. They are reentrant if callers do not share a range across threads.
