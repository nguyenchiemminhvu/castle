# P2Quantile

## Overview
`castle::statistic::p2_quantile<T>` estimates one quantile of a data stream in O(1) time and O(1) memory using the P² ("P-square") algorithm of Jain and Chlamtac (1985). Five markers follow the minimum, the target quantile, two intermediate quantiles, and the maximum. Every sample shifts the marker positions, and the marker heights are corrected with a parabolic prediction (or a linear one when the parabola would break the ordering of the markers). No sample is stored, so it suits unbounded streams where a sample window or a histogram is too large, or where the value range is not known in advance. In GNSS work it can follow, for example, the 95th percentile of receiver-reported `hAcc` over a whole drive. Note that this describes the receiver's own accuracy estimate, not the true position error.

## Header
`#include "castle_ext/statistic/p2_quantile.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../castle/core/compiler.md)
- [`castle/core/status.hpp`](../../castle/core/status.md)
- [`castle/core/traits.hpp`](../../castle/core/traits.md)
- [`castle/core/types.hpp`](../../castle/core/types.md)
- [`castle/dsa/array.hpp`](../../castle/dsa/array.md)
- [`castle_ext/statistic/validation.hpp`](validation.md)
- [`castle_ext/statistic/order_statistics.hpp`](order_statistics.md)

## Public API

### `template <typename T = double> class p2_quantile`

`T` must be an arithmetic type. `result_type` is `result_t<T>` (`T` for floating-point samples, `double` for integral samples). `size_type` is `castle::size_type`.

| Member | Signature | Description | Returns | Time | Stack |
|---|---|---|---|---|---|
| Default constructor | `p2_quantile()` | Creates an unconfigured estimator. | — | O(1) | O(1) |
| Constructor | `explicit p2_quantile(result_type probability)` | Calls `configure(probability)`. An invalid probability leaves it unconfigured. | — | O(1) | O(1) |
| `configure` | `status configure(result_type probability)` | Selects the quantile in the open interval (0, 1) and discards all samples. | `ok`, `invalid_config` (0, 1, NaN, negative, above 1; the estimator becomes unconfigured) | O(1) | O(1) |
| `configured` | `bool configured() const` | Reports whether a valid probability is set. | `bool` | O(1) | O(1) |
| `probability` | `result_type probability() const` | Configured probability; 0 when unconfigured. | value | O(1) | O(1) |
| `count` | `size_type count() const` | Number of samples seen. Saturates at the maximum `size_type`. | value | O(1) | O(1) |
| `empty` | `bool empty() const` | `true` when no sample was seen. | `bool` | O(1) | O(1) |
| `push` | `status push(T sample)` | Feeds one sample. | `ok`, `not_configured`, `invalid_argument` (NaN or infinity) | O(1) | O(1) |
| `quantile` | `status quantile(result_type& out) const` | Current estimate. Up to and including the fifth sample it is the exact linear-interpolation quantile of the samples seen (Hyndman–Fan type 7). From the sixth sample on it is the height of the middle marker. | `ok`, `not_configured`, `empty` | O(1) (at most 5 samples, already sorted) | O(1) |
| `clear` | `void clear()` | Discards all samples. The probability is kept. | — | O(1) | O(1) |

When `quantile()` fails, `out` is left unchanged.

## Usage Example
```cpp
#include "castle_ext/statistic/p2_quantile.hpp"

castle::statistic::p2_quantile<double> p95(0.95);
for (int i = 1; i <= 100; ++i)
{
    p95.push(static_cast<double>(i));
}
double estimate = 0.0;
if (p95.quantile(estimate) == castle::status::ok)
{
    // estimate is close to 95
}
```

See [`samples/castle_ext/statistic/p2_quantile.cpp`](../../../samples/castle_ext/statistic/p2_quantile.cpp) for a complete program.

## Constraints & Notes
- Storage is 20 values of `result_type` (heights, positions, desired positions, increments) plus a probability, a counter, and a flag. There is no heap allocation and no sample history.
- `push()` and `quantile()` take O(1) time and O(1) stack. A `push()` makes a fixed number of passes over five markers.
- The result is an estimate. Accuracy improves with the number of samples. Quantiles near 0 or 1 settle slowly. Use [`sorted_window`](sorted_window.md) when exact values over a bounded window are required.
- Up to five samples the markers are the sorted samples themselves, so `quantile()` reports their exact quantile. From the sixth sample on the middle marker tracks the configured probability.
- Floating-point NaN and infinity are rejected with `status::invalid_argument` and do not change the state. Integral samples are always accepted and are converted to `double`.
- `configure()` always discards the samples, even when it fails.
- The class is not thread-safe. Protect shared instances externally.
