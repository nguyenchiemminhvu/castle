# SortedWindow

## Overview
`castle::statistic::sorted_window<T, Capacity>` gives exact order statistics over the newest `Capacity` samples. It keeps every sample twice: once in arrival order (a [`sample_window`](sample_window.md)), so it knows which sample leaves next, and once in ascending order. A push finds the position of the evicted sample and of the new sample in the sorted copy by binary search, then shifts only the elements between the two positions. After that, the median, any quantile, the minimum, the maximum, the range, and the interquartile range (IQR) are O(1) reads of the sorted copy.

Use it when the window is short enough that exact answers are affordable, for example the last 100 horizontal errors of a GNSS receiver where you want the exact 95% CEP radius. For long windows, [`sliding_histogram`](sliding_histogram.md) keeps the cost of a push at O(1) in exchange for bin-resolution answers.

## Header
`#include "castle_ext/statistic/sorted_window.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../castle/core/compiler.md)
- [`castle/core/status.hpp`](../../castle/core/status.md)
- [`castle/core/traits.hpp`](../../castle/core/traits.md)
- [`castle/core/types.hpp`](../../castle/core/types.md)
- [`castle/dsa/algorithm.hpp`](../../castle/dsa/algorithm.md)
- [`castle/dsa/array.hpp`](../../castle/dsa/array.md)
- [`castle/dsa/array_view.hpp`](../../castle/dsa/array_view.md)
- [`castle_ext/statistic/validation.hpp`](validation.md)
- [`castle_ext/statistic/order_statistics.hpp`](order_statistics.md)
- [`castle_ext/statistic/sample_window.hpp`](sample_window.md)

## Public API

### `sorted_window<T, Capacity = 100>`

`T` is an arithmetic type and `Capacity` must be greater than zero. `result_type` is `T` for floating-point samples and `double` for integral samples. On failure, every `out` parameter is left unchanged.

| Member | Description | Returns | Time | Stack |
|---|---|---|---|---|
| `sorted_window()` | Constructs an empty window. | | O(1) | O(1) |
| `static capacity()` | Window length. | `Capacity` | O(1) | O(1) |
| `count()`, `empty()`, `full()` | Number of samples; no samples; `Capacity` samples. | `size_type` / `bool` | O(1) | O(1) |
| `push(T sample)` | Adds a sample; once the window is full, the oldest sample is evicted. Floating-point `T` rejects NaN and infinity. | `ok`, `invalid_argument` | O(log N) comparisons + O(N) moves worst case | O(1) |
| `clear()` | Removes every sample. | | O(1) | O(1) |
| `quantile(p, out, method = linear)` | Exact quantile for `p` in [0, 1]; see [`quantile_method`](order_statistics.md). | `ok`, `invalid_argument`, `empty` | O(1) | O(1) |
| `median(out)` | Linear quantile at 0.5. | `ok`, `empty` | O(1) | O(1) |
| `interquartile_range(out, method = linear)` | `Q3 - Q1`. | `ok`, `empty` | O(1) | O(1) |
| `minimum(out)`, `maximum(out)`, `range(out)` | Smallest sample, largest sample, `maximum - minimum`. | `ok`, `empty` | O(1) | O(1) |
| `mode(out)` | Most frequent value by exact equality; ties go to the smallest value. | `ok`, `empty` | O(N) | O(1) |
| `cdf(value, out)` | Empirical CDF: the fraction of samples less than or equal to `value`. `-inf` gives 0 and `+inf` gives 1. | `ok`, `invalid_argument` (NaN `value`), `empty` | O(log N) | O(1) |
| `sorted()` | Ascending `array_view<const T>` of the window, for batch functions such as [`kernel_density`](kernel_density.md). Invalidated by `push()` and `clear()`. | view | O(1) | O(1) |
| `samples()` | The retained samples in arrival order (`sample_window`). | `const sample_window&` | O(1) | O(1) |

A NaN `value` in `cdf()` is reported as `invalid_argument` before the emptiness check.

## Usage Example
```cpp
#include "castle_ext/statistic/sorted_window.hpp"

castle::statistic::sorted_window<float, 100U> radial_error;   // newest 100 fixes
(void)radial_error.push(1.2f);
(void)radial_error.push(0.8f);
(void)radial_error.push(2.1f);

float cep95 = 0.0f;
if (radial_error.quantile(0.95f, cep95, castle::statistic::quantile_method::nearest_rank) == castle::status::ok)
{
    // "95% of the recent fixes lie within cep95 metres."
}
```

The full program is in [`samples/castle_ext/statistic/sorted_window.cpp`](../../../samples/castle_ext/statistic/sorted_window.cpp).

## Constraints & Notes
- Storage is `2 * Capacity` samples plus the ring indices. All of it lives inside the object: no heap allocation and no recursion.
- Worst-case push cost is O(N) element moves when the new sample and the evicted sample sit at opposite ends of the sorted copy. The work depends on the distance between the two positions, not on `Capacity` alone. A stream whose values drift slowly is cheap.
- Equal values are inserted after existing equal values, so the sorted copy is stable with respect to arrival order.
- The mode uses exact equality, so it is meaningful only for discrete data (integers, quantised readings).
- Queries on an empty window return `status::empty` and leave `out` unchanged.
- Not thread-safe. Protect a shared instance with external synchronisation.
- The class satisfies the estimator contract used by [`cep`](cep.md): `push`, `quantile`, `cdf`, `count`, `empty`, `clear`.
- For GNSS data, a percentile of the true horizontal error is a CEP radius. A percentile of the receiver-reported `hAcc` describes the accuracy estimate, not the true error.
