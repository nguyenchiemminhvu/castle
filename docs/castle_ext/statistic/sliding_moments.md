# sliding_moments

## Overview
O(1) mean, variance, standard deviation, RMS, skewness, and excess kurtosis over the newest `Capacity` samples. Each push adds the new sample to a `central_moments` accumulator and, once the window is full, retracts the evicted sample. It rejects NaN and infinity. In GNSS pipelines, use it for a moving view of speed, heading error, or receiver-reported accuracy.

Retraction cancels digits, so two safeguards keep results accurate:
- **Slow drift:** a second accumulator is built from scratch alongside. After `Capacity` evicting pushes it holds exactly the current window and replaces the first one, so drift spans at most `2 * Capacity` updates.
- **Catastrophic cancellation:** when a retraction shrinks the variance by more than `1 / sqrt(epsilon)` (a large outlier leaving the window), more than half of the significant digits are lost, and the accumulator is rebuilt from the window.

## Header
`#include "castle_ext/statistic/sliding_moments.hpp"`

## Dependencies
- [compiler](../../castle/core/compiler.md)
- [status](../../castle/core/status.md)
- [traits](../../castle/core/traits.md)
- [types](../../castle/core/types.md)
- [algorithm](../../castle/dsa/algorithm.md)
- [validation](validation.md)
- [running_moments](running_moments.md)
- [sample_window](sample_window.md)

## Public API
`sliding_moments<T, Capacity = 100, Order = 2>`: `T` arithmetic, `Capacity > 0`, `Order` 2, 3, or 4. Results use `result_t<T>`. Output parameters are unchanged on failure. Stack use is O(1) for every operation.

| API | Description / statuses | Time |
| --- | --- | --- |
| `capacity()` (static), `count()`, `empty()`, `full()` | State queries. | O(1) |
| `push(sample)` | `ok`, or `invalid_argument` for NaN/infinity (floating `T`). Evicts the oldest sample when full. | O(1); O(Capacity) on an outlier rebuild |
| `clear()` | Removes every sample. | O(1) |
| `mean(out)`, `rms(out)` | `ok` or `empty`. | O(1) |
| `sum()`, `sum_squares()` | Window sum and sum of squares; zero when empty. | O(1) |
| `population_variance(out)`, `population_stddev(out)` | `ok` or `empty`. | O(1) |
| `sample_variance(out)`, `sample_stddev(out)` | `ok`, `empty` without samples, or `insufficient_data` with one sample. | O(1) |
| `skewness(out)` (`Order >= 3`) | `ok`, `empty`, or `invalid_argument` for zero variance. | O(1) |
| `excess_kurtosis(out)` (`Order >= 4`) | `ok`, `empty`, or `invalid_argument` for zero variance. | O(1) |
| `minimum(out)`, `maximum(out)`, `range(out)` | `ok` or `empty`; scan the window. | O(Capacity) |
| `samples()` | Const reference to the retained [sample_window](sample_window.md). | O(1) |
| `moments()` | Const reference to the `central_moments` describing the window. | O(1) |

## Usage Example
See [`samples/castle_ext/statistic/sliding_moments.cpp`](../../../samples/castle_ext/statistic/sliding_moments.cpp).

```cpp
castle::statistic::sliding_moments<float, 50U> speed;
speed.push(12.5f);
float rms = 0.0f;
if (speed.rms(rms) == castle::status::ok)
{
    // RMS of the newest (up to) 50 samples.
}
```

## Constraints & Notes
- Storage: the sample window plus two `central_moments<result_t<T>, Order>`; no heap allocation, no exceptions.
- Use [sorted_window](sorted_window.md) when minimum, maximum, or percentiles are needed on every sample.
- Non-copyable (the window is non-copyable). Not thread-safe; protect shared instances externally.
- Variance is compared as `after^2 < before^2 * epsilon`, so a window that becomes exactly constant after a large spread also triggers a rebuild.
- Rebuild cost is paid once per outlier departure; ordinary pushes stay O(1).
