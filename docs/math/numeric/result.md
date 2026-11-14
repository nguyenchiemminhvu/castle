# Numeric Result

## Overview
`castle::math::numeric_result<T>` is the common outcome record of iterative numeric algorithms (Romberg integration, root finders). Instead of throwing or writing through out-parameters, an algorithm returns the best estimate together with an error estimate, the number of iterations consumed and a `castle::status` telling whether the requested tolerance was reached. `castle::math::detail::within_tolerance()` is the shared convergence test used by those algorithms.

## Header
`#include "castle/math/numeric/result.hpp"`

## Dependencies
- [`castle/dsa/algorithm.hpp`](../../dsa/algorithm.md)
- [`castle/core/compiler.hpp`](../../core/compiler.md)
- [`castle/core/status.hpp`](../../core/status.md)
- [`castle/math/abs.hpp`](../abs.md)

## Public API
| Signature | Description |
| --- | --- |
| `template <typename T> struct numeric_result` | Aggregate result record for floating-point `T`; can be brace-initialized as `{value, error, iterations, code}`. |
| `T numeric_result<T>::value` | Best estimate of the sought quantity. Populated even when `code` reports a failure other than `status::invalid_argument`. |
| `T numeric_result<T>::error` | Estimated absolute error of `value`. |
| `unsigned numeric_result<T>::iterations` | Iterations (or refinement levels) consumed. |
| `castle::status numeric_result<T>::code` | Completion status: `status::ok` when the tolerance was met, `status::invalid_argument` for unusable input (e.g. a root bracket without a sign change), `status::out_of_range` when the iteration budget ran out first. |
| `constexpr bool numeric_result<T>::ok() const noexcept` | Returns `code == status::ok`. `O(1)`. |
| `template <typename T> constexpr bool detail::within_tolerance(T error, T value, T tolerance) noexcept` | Returns `error <= tolerance * max(1, abs(value))`, i.e. the mixed absolute/relative scaling used by `near_equal()`. Internal helper used by the iterative algorithms. `O(1)`. |

## Usage Example
```cpp
#include "castle/math/numeric/result.hpp"

castle::math::numeric_result<double> r = {1.0, 1e-9, 5U, castle::status::ok};
if (r.ok())
{
    // use r.value
}
```

See [`samples/math/numeric/result.cpp`](../../../samples/math/numeric/result.cpp).

## Constraints & Notes
- `numeric_result<T>` is a plain aggregate; it has no constructors and no invariants beyond what the producing algorithm documents.
- `ok()` only inspects `code`; a result with `status::out_of_range` still carries a usable best estimate.
- `detail::within_tolerance()` is an implementation detail: it is public for reuse by sibling numeric headers but is not a stable user-facing API. The error is expected to be non-negative.
- For `|value| <= 1` the tolerance is absolute; for larger magnitudes it scales with `|value|`.
