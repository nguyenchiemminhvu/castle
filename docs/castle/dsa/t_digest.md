# t_digest

## Overview
Fixed-size merging t-digest: estimates **quantiles** (median, p95, p99, ...) and the **CDF** of a stream of measurements without storing them. Exact quantiles need every sample, sorted. A t-digest keeps at most `Capacity` weighted centroids `{mean, weight}` plus a small buffer, whatever the stream length. Its accuracy is best where it matters most: at the tails.

- `quantile(q)`: the value below which a fraction `q` of the samples falls.
- `cdf(x)`: the fraction of samples at or below `x`.
- `min()` and `max()` are exact; `mean()` is exact up to floating-point rounding.
- Two digests merge: per-minute digests roll up into an hourly one, or several receivers into one report.

Typical embedded uses:

- GNSS output jitter: CEP50 / CEP68 / CEP95 / CEP99 of `|arrival - expected|` per message, without a sample buffer.
- Position error radius of a stationary receiver (CEP and R95 from `sqrt(dx^2 + dy^2)`).
- Latency percentiles of a bus, a task, or a request handler; sensor noise bands.
- Fraction of samples that meet a limit, for example "messages within 2 ms": `cdf(2.0)`.

How it works (Dunning's merging t-digest):

1. `add()` appends the sample to a buffer of `BufferSize` entries. Each entry is a centroid of weight 1.
2. When the buffer is full, `compress()` sorts all entries by mean (heapsort, in place) and merges neighbours in one pass from left to right.
3. Two neighbours merge only if the merged centroid spans at most one unit of the `k1` scale function `k(q) = delta / (2 pi) * asin(2q - 1)`. `k` is steep near `q = 0` and `q = 1`, so tail centroids stay tiny (often single samples) and middle centroids grow large. With `delta = Capacity - 1`, every pass ends with at most `Capacity` centroids.
4. `quantile()` and `cdf()` interpolate linearly between `(0, min)`, the centroid centers in rank space, and `(total, max)`. While every centroid still holds one sample, the result is the exact interpolation of the sorted samples.

Theory: [Dunning, Ertl: Computing Extremely Accurate Quantiles Using t-Digests](https://arxiv.org/abs/1902.04023), [Dunning: The t-digest: Efficient estimates of distributions](https://www.sciencedirect.com/science/article/pii/S2665963820300403), [Redis: t-digest](https://redis.io/docs/latest/develop/data-types/probabilistic/t-digest/).

## Header
`#include "castle/dsa/t_digest.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [constants](../core/constants.md) (`math::tau`)
- [status](../core/status.md) (`castle::status` results)
- [traits](../core/traits.md) (`meta::is_floating_point`)
- [type_ranges](../core/type_ranges.md) (`castle::numeric_limits`)
- [types](../core/types.md)
- [algorithm](algorithm.md) (`castle::sort`, heapsort)
- [array](array.md) (centroid storage)
- [trigonometry](../math/linalg/trigonometry.md) (`math::asin`, `math::sin` for the `k1` scale)

## Public API
### `t_digest<T = float, Capacity = 64, BufferSize = Capacity>`
| API | Description |
| --- | --- |
| `t_digest<T, Capacity, BufferSize>` | At most `Capacity` merged centroids plus `BufferSize` buffered samples, of floating-point type `T`. Requires `Capacity >= 2` and `BufferSize >= 1`. |
| `value_type`, `weight_type`, `size_type` | `T`, `uint32_t`, `castle::size_type`. |
| `centroid_type` | `{ mean, weight }`: the mean of a cluster of samples and how many there are. |
| `const_iterator` | `const centroid_type*`. |
| `t_digest()` | Empty digest. |
| `static constexpr size_type capacity()`, `buffer_size()` | `Capacity`, `BufferSize`. |
| `static constexpr T compression()` | `delta = Capacity - 1`. |
| `static constexpr uint32_t max_weight()` | Largest total weight, `UINT32_MAX`. |
| `status add(T value, uint32_t weight = 1)` | Adds `value` `weight` times. Returns `status::invalid_argument` for NaN or infinity, `status::full` when the total would exceed `max_weight()`, or `status::ok`. `weight == 0` changes nothing. The digest is unchanged on failure. |
| `status merge(const t_digest& other)` | Adds every sample of `other`; `other` may be `*this`. Returns `status::full` (digest unchanged) when the total would exceed `max_weight()`, else `status::ok`. |
| `void compress()` | Merges the buffer into the centroids. Afterwards `begin() .. end()` holds at most `Capacity` centroids sorted by mean. No-op when nothing is buffered. |
| `T quantile(T q)` | Estimated `q`-quantile; `q` is clamped to `[0, 1]`, NaN counts as 0. `quantile(0) == min()`, `quantile(1) == max()`. Zero when empty. Calls `compress()`. |
| `T cdf(T value)` | Estimated fraction of samples `<= value`: 0 below `min()` (and for NaN), 1 at or above `max()`. Zero when empty. Calls `compress()`. |
| `T mean() const` | Mean of every sample, buffered ones included. Zero when empty. |
| `T min() const`, `T max() const` | Exact smallest and largest sample. Zero when empty. |
| `uint32_t total() const` | Total weight: the number of samples added. |
| `bool empty() const` | `total() == 0`. |
| `size_type size() const` | Stored entries (centroids plus buffered samples). The centroid count after `compress()`. |
| `void clear()` | Forgets every sample. O(1). |
| `const_iterator begin() const`, `end() const` | Stored entries. Call `compress()` first for the sorted centroids only. |

## Usage Example
```cpp
#include "castle/dsa/t_digest.hpp"

// 64 centroids + 64 buffered samples: about 1 KiB for any number of messages.
castle::dsa::t_digest<float, 64U> jitter_ms;

// Per GNSS message: jitter amplitude in milliseconds.
if (jitter_ms.add(amplitude_ms) != castle::status::ok)
{
    // NaN or infinity from the clock source, or 4 billion samples reached: clear() per report period.
}

// Per report period:
const float cep50 = jitter_ms.quantile(0.50F);
const float cep68 = jitter_ms.quantile(0.68F);
const float cep95 = jitter_ms.quantile(0.95F);
const float cep99 = jitter_ms.quantile(0.99F);
const float within_budget = jitter_ms.cdf(2.0F); // fraction of messages within 2 ms

// Roll-up: hourly = sum of the minute digests.
castle::dsa::t_digest<float, 64U> hourly;
hourly.merge(jitter_ms);
jitter_ms.clear();
```

`samples/castle/dsa/t_digest.cpp` summarizes one hour of GNSS message jitter with about 3 % late messages, reads CEP50 / CEP68 / CEP95 / CEP99 and the share within a 2 ms budget, and merges the digests of two receivers.

## Constraints & Notes
- No allocation, exceptions, RTTI, or virtual dispatch. The object is one `dsa::array<centroid_type, Capacity + BufferSize>` (8 bytes per entry for `float`, 16 for `double`), two sizes, one `uint32_t` total, and the minimum and maximum. `t_digest<float, 64>` is 1056 bytes. The digest is copyable.
- **Accuracy.** The error is in **rank**: the true fraction below `quantile(q)` is close to `q`. Measured on 200 000 samples with `t_digest<float, 64>` (half-normal and exponential, jitter-like):

  | Quantile | Rank error | Relative value error |
  | --- | --- | --- |
  | p50, p68, p95 | at most 0.2 % | 0.06 % to 0.5 % |
  | p99 | at most 0.05 % | 0.6 % to 0.7 % |
  | p99.9 | at most 0.03 % | 0.8 % to 5 % |

  `Capacity = 100` roughly halves the value errors (p99: 0.26 %, p99.9: 0.5 % on the half-normal stream). The value error grows where the density is thin, as in the far tail of the exponential stream. A gap in the data (two separate groups) is smoothed by the centroid that straddles it.
- **Timing.** `add()` is O(1), plus one `compress()` every `BufferSize` samples. `compress()` is O(n log n) for `n = Capacity + BufferSize`, plus one `asin` and one `sin` per output centroid; the merge pass itself is O(n). `quantile()`, `cdf()`, and `mean()` are O(Capacity) after the merge. A larger `BufferSize` makes `add()` cheaper on average; it does not change the accuracy much.
- **Bounded storage.** The merge pass writes in place and never outputs more entries than it reads. A guard caps the output at `Capacity` centroids even under floating-point rounding, so the buffer always has room.
- **Precision.** Centroid means and the scale function use `T`. Ranks in `quantile()` and `cdf()` use `double`, so they stay exact for every total. Use `double` samples when values span many orders of magnitude.
- **Weights.** Weights are `uint32_t`; the total is limited to `UINT32_MAX` samples and `add()` / `merge()` return `status::full` beyond it. Call `clear()` per report period for endless streams.
- **Non-finite input.** `add()` rejects NaN and infinities, which would break the ordering of the centroids. The check relies on IEEE arithmetic; do not build with `-ffast-math`.
- **const queries.** `quantile()` and `cdf()` merge the buffer first, so they are not `const`. `mean()`, `min()`, `max()`, and `total()` are `const`.
- **Persistence.** After `compress()`, replay each centroid with `add(mean, weight)` to rebuild an equivalent digest (`min()` and `max()` then become the extreme centroid means).
- Not thread-safe.
