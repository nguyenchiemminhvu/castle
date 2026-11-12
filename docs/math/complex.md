# Complex

## Overview
`castle::math::complex<T>` is an allocation-free complex number that stores a Cartesian real/imaginary pair of a floating-point type. It provides the usual arithmetic with complex and real operands, plus magnitude, phase, polar construction, conjugate, exponential, logarithm, principal square root, integer powers, and a tolerance comparison. Division uses Smith's algorithm to avoid spurious overflow, and `abs()` is built on `castle::math::hypot()`. Use it for signal-processing and control code on targets without the STL.

## Header
`#include "castle/math/complex.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/error_handler.hpp`](../core/error_handler.md)
- [`castle/core/traits.hpp`](../core/traits.md)
- [`castle/dsa/algorithm.hpp`](../dsa/algorithm.md)
- [`castle/math/abs.hpp`](abs.md)
- [`castle/math/exponential.hpp`](exponential.md)
- [`castle/math/hypot.hpp`](hypot.md)
- [`castle/math/linalg/trigonometry.hpp`](linalg/trigonometry.md)
- [`castle/math/powi.hpp`](powi.md)
- [`castle/math/sqrt_real.hpp`](sqrt_real.md)

## Public API
All symbols live in `castle::math`. `T` must be a floating-point type (`static_assert`). Members and operators marked "constexpr" are usable in constant expressions; `arg`, `polar`, `exp`, `log`, `sqrt` and `pow` are not `constexpr` because they call the C math library.

### Types
| Signature | Description |
| --- | --- |
| `template <typename T> class complex` | Complex number with `T` real and imaginary parts. Trivially cheap value type, no heap, no exceptions. |
| `using value_type = T` | Component type. |
| `using complexf = complex<float>` | Single-precision alias. |
| `using complexd = complex<double>` | Double-precision alias. |

### Construction, access, assignment
| Signature | Description |
| --- | --- |
| `constexpr complex(T real = 0, T imag = 0) noexcept` | Builds `real + imag*i`; implicit from a real value, default is `0 + 0i`. `O(1)`. |
| `template <typename U> constexpr explicit complex(const complex<U>& other) noexcept` | Converts between component types (e.g. `complexf` <-> `complexd`) with `static_cast`. Narrowing loses precision. `O(1)`. |
| `constexpr T real() const noexcept` | Returns the real part. `O(1)`. |
| `constexpr T imag() const noexcept` | Returns the imaginary part. `O(1)`. |
| `constexpr void real(T value) noexcept` | Replaces the real part. `O(1)`. |
| `constexpr void imag(T value) noexcept` | Replaces the imaginary part. `O(1)`. |
| `constexpr complex operator+() const noexcept` | Returns the value unchanged. `O(1)`. |
| `constexpr complex operator-() const noexcept` | Returns the negated value. `O(1)`. |
| `constexpr complex& operator=(T value) noexcept` | Assigns a real value and clears the imaginary part. Returns `*this`. `O(1)`. |

### Compound assignment
| Signature | Description |
| --- | --- |
| `constexpr complex& operator+=(T value)` / `operator-=(T value)` | Adds/subtracts a real value to/from the real part. Returns `*this`. `O(1)`. |
| `constexpr complex& operator*=(T value)` | Scales both components. Returns `*this`. `O(1)`. |
| `constexpr complex& operator/=(T value)` | Divides both components; `value` must not be zero (`CASTLE_ASSERT`). Returns `*this`. `O(1)`. |
| `constexpr complex& operator+=(const complex& other)` / `operator-=(const complex& other)` | Component-wise add/subtract. Returns `*this`. `O(1)`. |
| `constexpr complex& operator*=(const complex& other)` | Complex product `(a+bi)(c+di) = (ac-bd) + (ad+bc)i`; safe for `z *= z`. Returns `*this`. `O(1)`. |
| `constexpr complex& operator/=(const complex& other)` | Complex quotient using Smith's algorithm (branch on `abs(c) >= abs(d)`); `other` must not be zero (`CASTLE_ASSERT`). Returns `*this`. `O(1)`. |

### Binary operators
| Signature | Description |
| --- | --- |
| `operator+(complex<T>, const complex<T>&)`, `operator+(complex<T>, T)`, `operator+(T, const complex<T>&)` | Sum with complex, real-right and real-left operands. The real operand is a non-deduced context (`type_identity_t<T>`), so `z + 1` with `z` of type `complexf` works for any convertible scalar. `constexpr`, `O(1)`. |
| `operator-(complex<T>, const complex<T>&)`, `operator-(complex<T>, T)`, `operator-(T, const complex<T>&)` | Difference with complex, real-right and real-left operands. `constexpr`, `O(1)`. |
| `operator*(complex<T>, const complex<T>&)`, `operator*(complex<T>, T)`, `operator*(T, complex<T>)` | Product with complex, real-right and real-left operands. `constexpr`, `O(1)`. |
| `operator/(complex<T>, const complex<T>&)`, `operator/(complex<T>, T)`, `operator/(T, const complex<T>&)` | Quotient with complex, real-right and real-left operands; the divisor must not be zero. `constexpr`, `O(1)`. |
| `operator==(const complex<T>&, const complex<T>&)`, `operator==(const complex<T>&, T)` | Exact component-wise equality; with a real right operand the imaginary part must equal `0`. `constexpr`, `O(1)`. |
| `operator!=(const complex<T>&, const complex<T>&)`, `operator!=(const complex<T>&, T)` | Negation of the matching `operator==`. `constexpr`, `O(1)`. |

### Free functions
| Signature | Description |
| --- | --- |
| `constexpr T real(const complex<T>& value) noexcept` | Real part of `value`. `O(1)`. |
| `constexpr T imag(const complex<T>& value) noexcept` | Imaginary part of `value`. `O(1)`. |
| `constexpr complex<T> conj(const complex<T>& value) noexcept` | Complex conjugate `re - im*i`. `O(1)`. |
| `constexpr T norm(const complex<T>& value) noexcept` | Squared magnitude `re^2 + im^2`; no square root, may overflow above `sqrt(max())`. `O(1)`. |
| `constexpr T abs(const complex<T>& value) noexcept` | Magnitude via `hypot(re, im)`; avoids intermediate overflow. `O(1)`. |
| `T arg(const complex<T>& value) noexcept` | Phase angle `atan2(im, re)` in radians, in `[-pi, pi]`. `O(1)`. |
| `complex<T> polar(T magnitude, T phase = 0) noexcept` | Builds `magnitude * (cos(phase) + i sin(phase))`; `phase` in radians. `O(1)`. |
| `complex<T> exp(const complex<T>& value) noexcept` | `e^value = e^re * (cos(im) + i sin(im))`. `O(1)`. |
| `complex<T> log(const complex<T>& value) noexcept` | Principal logarithm `log\|value\| + i*arg(value)`; imaginary part in `[-pi, pi]`. `value` must not be zero (`CASTLE_ASSERT`). `O(1)`. |
| `complex<T> sqrt(const complex<T>& value) noexcept` | Principal square root with non-negative real part. Returns `0` for zero input; uses a cancellation-free formula separately for `re >= 0` and `re < 0` (the sign of `im` selects the sign of the result's imaginary part). `O(1)`. |
| `complex<T> pow(const complex<T>& base, int exponent) noexcept` | Integer power by repeated squaring (`powi`) in `O(log\|exponent\|)` multiplications; negative exponents return the reciprocal of `base^\|exponent\|`, with the magnitude formed in unsigned arithmetic so `INT_MIN` is well defined. A zero base with a negative exponent divides by zero. |
| `constexpr bool near_equal(const complex<T>& a, const complex<T>& b, T epsilon) noexcept` | Returns `abs(a - b) <= epsilon * max(1, abs(a), abs(b))`. Complex counterpart of the scalar [`near_equal`](near_equal.md). `O(1)`. |

## Usage Example
```cpp
#include "castle/math/complex.hpp"

using castle::math::complexd;

const complexd z(3.0, 4.0);
const double magnitude = castle::math::abs(z);                 // 5
const complexd rotated = z * castle::math::polar(1.0, 0.5);    // rotate by 0.5 rad
const complexd root = castle::math::sqrt(complexd(-4.0, 0.0)); // 2i
const complexd cubed = castle::math::pow(z, 3);
const bool same = castle::math::near_equal(root * root, complexd(-4.0, 0.0), 1.0e-9);
```

See [`samples/math/complex.cpp`](../../samples/math/complex.cpp).

## Constraints & Notes
- Floating-point component types only; other types fail a `static_assert`.
- Operators with a real operand take it as `type_identity_t<T>`, so a literal of a different arithmetic type converts implicitly; the complex operand determines `T`. Conversion between `complexf` and `complexd` is explicit.
- Division by zero (`operator/=`, `operator/`, `log` of zero) is a precondition violation reported through `CASTLE_ASSERT`; `pow` with a zero base and negative exponent shares the same restriction.
- `operator==` / `operator!=` are exact comparisons; use `near_equal` for computed values. The real-operand overloads treat the number as `value + 0i`.
- `sqrt` assigns the sign of the imaginary part from `imag < 0`; a negative-zero imaginary part is treated as non-negative, so `sqrt(-4 - 0i)` is `+2i`.
- `exp`, `log`, `arg`, `polar` rely on the C math library via [`exponential.hpp`](exponential.md) and [`trigonometry.hpp`](linalg/trigonometry.md), so their accuracy follows the platform and they are not `constexpr`.
- `norm` is cheaper than `abs` but can overflow for large components; prefer `abs` when the range is unknown.
