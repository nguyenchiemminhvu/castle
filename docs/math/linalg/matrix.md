# Fixed-size mathematical matrix

## Overview

A fixed-size row-major matrix for embedded linear algebra, numeric processing and homogeneous transformations. Matrix dimensions are compile-time constants and storage is contiguous.

## Header

```cpp
#include "castle/math/linalg/matrix.h"
```

**Namespace:** `castle::math`

## Main API

| API | Purpose |
|---|---|
| `matrix<T, Rows, Columns>` | Fixed-size row-major matrix with arithmetic elements. |
| `operator()` / `at()` | Two-dimensional element access. |
| `row()` / `column()` | Extract rows and columns as fixed-size vectors. |
| `transpose()` / `transpose()` free function | Swap matrix dimensions. |
| Scalar operators | Multiply or divide every element by a scalar. |
| Matrix addition/subtraction | Element-wise matrix arithmetic with common-type promotion for free operators. |
| Matrix multiplication | Standard matrix product; the inner dimensions must match. |
| Matrix-vector multiplication | Apply a matrix to a column vector. |
| `hadamard()` / `outer_product()` | Element-wise matrix multiplication and vector outer product. |
| `frobenius_squared()` / `trace()` | Common scalar matrix reductions. |
| `determinant()` | Determinant of a square fixed-size matrix. |
| `try_inverse()` / `inverse()` | Floating-point matrix inversion with a configurable singularity tolerance. |
| `identity()` / `diagonal_matrix()` / `skew_symmetric()` | Common matrix constructors. |
| `matrix2x2` / `matrix3x3` / `matrix4x4` | Common dimension aliases. |

## Example

```cpp
using castle::math::matrix;
using castle::math::vector;

const matrix<float, 3U, 3U> rotation(
    0.0F, -1.0F, 0.0F,
    1.0F,  0.0F, 0.0F,
    0.0F,  0.0F, 1.0F);

const vector<float, 3U> point(1.0F, 0.0F, 1.0F);
const vector<float, 3U> result = rotation * point;
```

## Embedded notes

- Storage is a plain embedded array in row-major order.
- Matrix multiplication and matrix-vector multiplication use the conventional column-vector model: `result = matrix * vector`.
- Arithmetic operators do not allocate and do not rely on STL containers.
- `try_inverse()` uses floating-point Gauss-Jordan elimination with partial pivoting and returns `false` when a pivot is numerically singular.
- `inverse()` asserts when inversion is not possible; use `try_inverse()` when failure must be handled explicitly.
- `determinant()` is intended for small fixed matrices, where predictable bounded work is preferable to general-purpose dynamic linear-algebra algorithms.
