# SlidingHistogram

## Overview
`castle::statistic::sliding_histogram<T, Capacity, BinCount>` gives approximate quantiles, CDF, and mode over the newest `Capacity` samples. It pairs a [`sample_window`](sample_window.md) with a fixed-range [`histogram`](histogram.md). Each push adds the new sample to its bin and retracts the evicted sample from its bin, so a push costs O(1). Queries cost O(BinCount) and are accurate to one bin width.

Compared with the exact [`sorted_window`](sorted_window.md), it stores the window once instead of twice and never shifts elements, which suits long windows (for example one hour of 1 Hz GNSS errors). The price is bin resolution instead of exact values.

## Header
`#include "castle_ext/statistic/sliding_histogram.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../castle/core/compiler.md)
- [`castle/core/status.hpp`](../../castle/core/status.md)
- [`castle/core/traits.hpp`](../../castle/core/traits.md)
- [`castle/core/types.hpp`](../../castle/core/types.md)
- [`castle_ext/statistic/validation.hpp`](validation.md)
- [`castle_ext/statistic/histogram.hpp`](histogram.md)
- [`castle_ext/statistic/sample_window.hpp`](sample_window.md)

## Public API

### `sliding_histogram<T, Capacity, BinCount>`

`T` is an arithmetic type; `Capacity` (window length) and `BinCount` must be greater than zero. `result_type` is `T` for floating-point samples and `double` for integral samples. `histogram_type` is `histogram<T, BinCount, smallest_uint_t<Capacity>, histogram_range::fixed>`, so the counters use the smallest unsigned type that can hold `Capacity`. On failure, every `out` parameter is left unchanged.

| Member | Description | Returns | Time | Stack |
|---|---|---|---|---|
| `sliding_histogram()` | Unconfigured instance; `push()` reports `not_configured`. | | O(BinCount) | O(1) |
| `sliding_histogram(lower, upper)` | Configures the histogram range `[lower, upper)`. Invalid bounds leave it unconfigured; check `configured()`. | | O(BinCount) | O(1) |
| `configure(lower, upper)` | Sets the range and discards every sample. | `ok`, `invalid_config` | O(BinCount) | O(1) |
| `configured()` | Whether a valid range is set. | `bool` | O(1) | O(1) |
| `static capacity()`, `count()`, `empty()`, `full()` | Window length; samples held; no samples; `Capacity` samples. | | O(1) | O(1) |
| `push(T sample)` | Adds a sample. When the window is full, the oldest sample is evicted and retracted from the histogram. Floating-point `T` rejects NaN and infinity. | `ok`, `not_configured`, `invalid_argument` | O(1) | O(1) |
| `clear()` | Removes every sample; the range is kept. | | O(BinCount) | O(1) |
| `quantile(p, out)` | Approximate quantile; see [`histogram::quantile()`](histogram.md). | `ok`, `not_configured`, `invalid_argument`, `empty` | O(BinCount) | O(1) |
| `median(out)` | Approximate quantile at 0.5. | `ok`, `not_configured`, `empty` | O(BinCount) | O(1) |
| `cdf(value, out)` | Approximate CDF; see [`histogram::cdf()`](histogram.md). | `ok`, `not_configured`, `invalid_argument`, `empty` | O(BinCount) | O(1) |
| `mode(out)` | Centre of the heaviest bin; the lowest bin wins ties. | `ok`, `not_configured`, `empty` | O(BinCount) | O(1) |
| `distribution()` | The underlying `const histogram_type&` (bins, underflow, overflow, edges). | reference | O(1) | O(1) |
| `samples()` | Retained samples in arrival order (`const sample_window&`). | reference | O(1) | O(1) |

When the window is not full, `push()` only adds to the histogram. Values outside the range stay in the underflow or overflow counters while they are in the window, and leave them when they are evicted.

## Usage Example
```cpp
#include "castle_ext/statistic/sliding_histogram.hpp"

// One hour of 1 Hz radial errors, 0.1 m resolution up to 25.6 m.
castle::statistic::sliding_histogram<float, 3600U, 256U> errors(0.0f, 25.6f);
(void)errors.push(1.3f);

float cep95 = 0.0f;
if (errors.quantile(0.95f, cep95) == castle::status::ok)
{
    // Approximate 95th percentile of the last hour.
}
```

The full program is in [`samples/castle_ext/statistic/sliding_histogram.cpp`](../../../samples/castle_ext/statistic/sliding_histogram.cpp).

## Constraints & Notes
- Storage is one `sample_window` of `Capacity` samples plus a histogram of `BinCount + 3` counters. All of it lives inside the object: no heap allocation and no recursion.
- Quantile resolution is one bin width, `(upper - lower) / BinCount`. Choose the range to cover the values you expect.
- `configure()` clears the window, because the retained samples would no longer match the new bins.
- Not thread-safe. Protect a shared instance with external synchronisation.
- The class satisfies the estimator contract used by [`cep`](cep.md): `push`, `quantile`, `cdf`, `count`, `empty`, `clear`.
- For GNSS data, a percentile of the true horizontal error is a CEP radius. A percentile of the receiver-reported `hAcc` describes the accuracy estimate, not the true error.
