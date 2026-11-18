# CEP

## Overview
`castle::statistic::cep<TEstimator>` computes the circular error probable (CEP) of a stream of position errors. CEPp is the radius of a circle, centred on the reference point, that contains p percent of the position fixes: CEP50 holds half of them and CEP95 holds 95 percent. The class wraps any quantile estimator that satisfies the contract below. It validates radial errors, converts 2-D and 3-D residuals to radii with `hypot`, and offers `cep50()`, `cep68()`, `cep90()`, `cep95()`, `cep99()`, arbitrary percentiles, and the fraction of fixes inside a given radius.

A typical GNSS use is to keep the latest 100 horizontal errors against a known reference, drop the oldest one for every new fix, and report CEP50/68/95/99. Percentiles of the receiver-reported horizontal accuracy (`hAcc`) describe the receiver's own accuracy estimate, not the true error. Only residuals against a surveyed reference give a true CEP.

## Header
`#include "castle_ext/statistic/cep.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../castle/core/compiler.md)
- [`castle/core/status.hpp`](../../castle/core/status.md)
- [`castle/core/traits.hpp`](../../castle/core/traits.md)
- [`castle/core/types.hpp`](../../castle/core/types.md)
- [`castle/math/hypot.hpp`](../../castle/math/hypot.md)
- [`castle/utility/forward.hpp`](../../castle/utility/forward.md)
- [`castle_ext/statistic/validation.hpp`](validation.md)
- [`castle_ext/statistic/histogram.hpp`](histogram.md)
- [`castle_ext/statistic/sliding_histogram.hpp`](sliding_histogram.md)
- [`castle_ext/statistic/sorted_window.hpp`](sorted_window.md)

## Public API

### Estimator contract
`TEstimator` must provide `value_type`, `result_type`, `push(value_type)`, `quantile(result_type probability, result_type& out) const`, `cdf(result_type value, result_type& out) const`, `count() const`, `empty() const`, and `clear()`.

### Ready-made aliases

| Alias | Estimator | Push | Query | Memory |
|---|---|---|---|---|
| `exact_cep<T = double, N = 100>` | [`sorted_window<T, N>`](sorted_window.md): newest N errors, exact | O(N) worst case | O(1) quantile, O(log N) fraction | about 2N samples |
| `sliding_histogram_cep<T, N, B>` | [`sliding_histogram<T, N, B>`](sliding_histogram.md): newest N errors, binned | O(1) | O(B) | N samples plus B counters |
| `cumulative_cep<T, B, TCount = uint32_t>` | expanding [`histogram<T, B, TCount>`](histogram.md): whole stream, binned | O(1) amortized | O(B) | B counters |

The histogram-based aliases are constructed with the radius range, for example `sliding_histogram_cep<double, 100U, 64U> tracker(0.0, 50.0);`. Pass the arguments of the estimator's constructor to the `cep` constructor.

### `template <typename TEstimator> class cep`

`value_type` and `result_type` are taken from the estimator. All query methods leave `out` unchanged when they fail.

| Member | Signature | Description | Returns | Time | Stack |
|---|---|---|---|---|---|
| Default constructor | `cep()` | Default-constructs the estimator. | — | estimator | O(1) |
| Forwarding constructor | `explicit cep(TArg&&, TArgs&&...)` | Constructs the estimator from the arguments, for example a histogram range. | — | estimator | O(1) |
| `push_distance` | `status push_distance(value_type distance)` | Adds a radial error. Floating-point distances must be finite and not negative; signed integral distances must not be negative; unsigned distances are always valid. | `ok`, `invalid_argument`, or the estimator's `push()` status | estimator `push()` | O(1) |
| `push_residual` (2-D) | `status push_residual(result_type east, result_type north)` | Adds the radius `hypot(east, north)`. Floating-point `value_type` only. | `ok`, `invalid_argument` (non-finite component), or the estimator's `push()` status | estimator `push()` + O(1) | O(1) |
| `push_residual` (3-D) | `status push_residual(result_type east, result_type north, result_type up)` | Adds the spherical radius `hypot(east, north, up)`. Floating-point `value_type` only. | same as above | estimator `push()` + O(1) | O(1) |
| `quantile` | `status quantile(result_type probability, result_type& out) const` | Radius containing the fraction `probability` of the errors. | estimator's `quantile()` status (`empty` without samples, `invalid_argument` outside [0, 1]) | estimator `quantile()` | O(1) |
| `percentile` | `status percentile(result_type percent, result_type& out) const` | Radius containing `percent` percent of the errors. | `invalid_argument` outside [0, 100] or for NaN, otherwise the `quantile()` status | estimator `quantile()` | O(1) |
| `cep50`, `cep68`, `cep90`, `cep95`, `cep99` | `status cepXX(result_type& out) const` | Radius containing 50, 68, 90, 95, 99 percent of the errors. | estimator's `quantile()` status | estimator `quantile()` | O(1) |
| `fraction_within` | `status fraction_within(result_type radius, result_type& out) const` | Fraction of errors at or below `radius` (empirical CDF). | estimator's `cdf()` status | estimator `cdf()` | O(1) |
| `count` | `auto count() const` | Number (or total weight) of errors held by the estimator. | value | O(1) | O(1) |
| `empty` | `bool empty() const` | `true` when the estimator holds no error. | `bool` | O(1) | O(1) |
| `clear` | `void clear()` | Discards every error. | — | estimator `clear()` | O(1) |
| `estimator` | `estimator_type& estimator()`, `const estimator_type& estimator() const` | Access to the wrapped estimator, for example to choose `quantile_method::nearest_rank`. | reference | O(1) | O(1) |

## Usage Example
```cpp
#include "castle_ext/statistic/cep.hpp"

// Horizontal accuracy of the latest 100 fixes.
castle::statistic::exact_cep<float, 100U> horizontal;
horizontal.push_residual(0.8f, -1.1f); // east/north error in metres
horizontal.push_distance(0.4f);        // radial error in metres

float cep95 = 0.0f;
if (horizontal.cep95(cep95) == castle::status::ok)
{
    // 95 % of the retained fixes lie within cep95 metres
}
```

See [`samples/castle_ext/statistic/cep.cpp`](../../../samples/castle_ext/statistic/cep.cpp) for a complete program including the GNSS scenario.

## Constraints & Notes
- `cep` holds no state of its own. All state lives in the estimator, so storage, complexity, and thread-safety follow the chosen alias. There is no heap allocation.
- `exact_cep` and `sliding_histogram_cep` keep the newest N errors and drop the oldest one for each new error once full. `cumulative_cep` keeps the whole stream and is limited only by the counter type `TCount`.
- `exact_cep` uses linear interpolation between order statistics by default. For 1..100, CEP95 is 95.05 and CEP50 is 50.5. For the "p % of fixes within r" convention, call `estimator().quantile(p, out, castle::statistic::quantile_method::nearest_rank)`.
- Histogram-based results are approximations: they are interpolated inside a bin, so choose the range and bin count to match the accuracy you need. Errors outside a fixed range count as underflow or overflow; see [`histogram`](histogram.md).
- `push_residual()` is available only for floating-point `value_type`; calling it for an integral type fails to compile, so no value is silently truncated.
- `push_distance()` rejects negative values, NaN, and infinity for floating-point types, and negative values for signed integral types. Unsigned types never fail validation.
- `percentile()` is `quantile(percent / 100)` with the extra range check on `percent`.
- Not thread-safe. Protect shared instances externally.
