# Histogram

## Overview
`castle::statistic::histogram<T, BinCount, TCount, Range>` is a weighted histogram with `BinCount` equal-width bins over a range `[lower, upper)` that you set at run time. It answers approximate questions about a stream without storing the samples: quantiles, the empirical CDF, and the mode (the centre of the heaviest bin). The answer resolution is one bin width.

Two range policies are available:

- `histogram_range::fixed` (default): a value below `lower` is counted in a separate underflow counter, and a value at or above `upper` in an overflow counter, so outliers never distort the edge bins. Weight can be retracted with `remove()`, which [`sliding_histogram`](sliding_histogram.md) relies on.
- `histogram_range::expanding`: auto-ranging with a fixed number of bins. When a value falls outside the range, the range doubles away from the value and adjacent bins merge pairwise until the value fits. Use it when the spread of the stream is not known up front, for example GNSS error radii.

For GNSS work, a histogram of horizontal errors gives CEP radii (the 50% and 95% quantiles) with constant memory. A histogram of the receiver-reported `hAcc` describes the accuracy estimate, not the true error.

## Header
`#include "castle_ext/statistic/histogram.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../castle/core/compiler.md)
- [`castle/core/status.hpp`](../../castle/core/status.md)
- [`castle/core/traits.hpp`](../../castle/core/traits.md)
- [`castle/core/type_ranges.hpp`](../../castle/core/type_ranges.md)
- [`castle/core/types.hpp`](../../castle/core/types.md)
- [`castle/dsa/algorithm.hpp`](../../castle/dsa/algorithm.md)
- [`castle/dsa/array.hpp`](../../castle/dsa/array.md)
- [`castle/math/clamp.hpp`](../../castle/math/clamp.md)
- [`castle_ext/statistic/validation.hpp`](validation.md)

## Public API

### Template parameters

| Parameter | Meaning |
|---|---|
| `T` | Arithmetic sample type. `result_type` is `T` for floating-point and `double` for integral samples. |
| `BinCount` | Number of bins, greater than zero. Must be even (and at least 2) for `expanding`. |
| `TCount` | Weight/counter type: an unsigned integer (default `uint32_t`) or a floating-point type for real-valued weights. |
| `Range` | `histogram_range::fixed` (default) or `histogram_range::expanding`. |

### `histogram`

On failure, every `out` parameter is left unchanged.

| Member | Description | Returns | Time | Stack |
|---|---|---|---|---|
| `histogram()` | Unconfigured histogram. | | O(BinCount) | O(1) |
| `histogram(lower, upper)` | Constructs and configures. Invalid bounds leave it unconfigured; check `configured()`. | | O(BinCount) | O(1) |
| `configure(lower, upper)` | Sets the range and discards all weight. | `ok`; `invalid_argument` for a non-finite bound, `lower >= upper`, or a width that overflows (the histogram becomes unconfigured) | O(BinCount) | O(1) |
| `configured()`, `lower()`, `upper()`, `bin_width()` | Range queries. `bin_width()` is 0 when unconfigured. In `expanding` mode the bounds grow. | | O(1) | O(1) |
| `static bin_count()`, `range_policy` | Compile-time bin count and policy. | | O(1) | O(1) |
| `count()`, `empty()`, `underflow()`, `overflow()` | Total weight including underflow and overflow; no weight; weight below `lower()`; weight at or above `upper()`. | | O(1) | O(1) |
| `push(value, weight = 1)` | Adds a sample with a positive weight. | `ok`; `not_configured`; `invalid_argument` for a non-finite value or a zero, negative, or non-finite weight; `full` when an integral total would overflow | `fixed`: O(1). `expanding`: O(1), plus O(BinCount) per range doubling (at most about 2100 doublings for a finite `double`) | O(1) |
| `push_range(first, last)` | Pushes every element with weight 1; stops at the first failure and keeps the earlier samples. | `ok` or the first failing `push()` status | O(N) fixed | O(1) |
| `remove(value, weight = 1)` | Retracts weight previously added. `fixed` only (compile-time check). | `ok`; `not_configured`; `invalid_argument`; `not_found` when the slot holds less than `weight` | O(1) | O(1) |
| `clear()` | Discards all weight; the range is kept. | | O(BinCount) | O(1) |
| `bin(index, out)` | Weight of one bin. | `ok`, `out_of_range` | O(1) | O(1) |
| `bin_edges(index, lower, upper)` | Edges `[lower, upper)` of one bin. | `ok`, `not_configured`, `out_of_range` | O(1) | O(1) |
| `mode(out)` | Centre of the heaviest bin; the lowest bin wins ties. Underflow and overflow are ignored. | `ok`, `not_configured`, `empty` (no in-range weight) | O(BinCount) | O(1) |
| `quantile(p, out)` | Approximate quantile for `p` in [0, 1], interpolating linearly inside the bin that holds it. Returns `lower()` when it falls inside the underflow weight and `upper()` when it falls inside the overflow weight. | `ok`, `not_configured`, `invalid_argument`, `empty` | O(BinCount) | O(1) |
| `cdf(value, out)` | Approximate fraction of the weight at or below `value`. Below `lower()` the result is 0, and at or above `upper()` it is `1 - overflow / count`. | `ok`, `not_configured`, `invalid_argument` (non-finite `value`), `empty` | O(BinCount) | O(1) |

The checks run in this order: `not_configured` first, then argument validation, then `empty`.

## Usage Example
```cpp
#include "castle_ext/statistic/histogram.hpp"

castle::statistic::histogram<double, 50U> errors(0.0, 10.0);   // 0.2 m bins
(void)errors.push(1.1);
(void)errors.push(2.9, 3U);   // weighted: three equal observations

double cep95 = 0.0;
if (errors.quantile(0.95, cep95) == castle::status::ok)
{
    // Approximate 95th percentile; the resolution is one bin (0.2 m).
}

// Unknown spread: let the range grow.
castle::statistic::histogram<double, 8U, uint32_t, castle::statistic::histogram_range::expanding> radius(0.0, 1.0);
(void)radius.push(5.0);   // range becomes [0, 8)
```

The full program is in [`samples/castle_ext/statistic/histogram.cpp`](../../../samples/castle_ext/statistic/histogram.cpp).

## Constraints & Notes
- Storage is `BinCount + 3` counters, two bounds, and a flag. All of it lives inside the object: no heap allocation and no recursion.
- Quantiles and the CDF assume that the weight of a bin is spread evenly across it. Underflow and overflow weight is placed at the range edges, so the CDF there is a conservative lower bound.
- In `expanding` mode the bins are merged pairwise on each doubling, so resolution gets coarser as the range grows. If doubling would make a bound or the width non-finite, the range stops growing and the value goes to the underflow or overflow counter.
- With an integral `TCount`, a push that would overflow the total returns `status::full`. Choose the counter width for the largest expected total.
- `remove()` is only for `fixed` ranges. It assumes the range has not changed since the weight was added.
- `T` may be integral. Finite checks apply to floating-point `T` and `TCount` only.
- Not thread-safe. Protect a shared instance with external synchronisation.
- The class satisfies the estimator contract used by [`cep`](cep.md): `push`, `quantile`, `cdf`, `count`, `empty`, `clear`.
