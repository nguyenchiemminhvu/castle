# Matrix

## Overview
`castle::math::matrix<T, Rows, Columns>` is a fixed-size row-major matrix for deterministic embedded linear algebra. Use it when matrix dimensions are known at compile time and you need contiguous inline storage, matrix/vector products, determinants, inversion, and transform-friendly homogeneous math without heap allocation or STL containers.

## Header
`#include "castle/math/linalg/matrix.hpp"`

## Dependencies
- [../../core/compiler.hpp](../../core/compiler.md)
- [../../core/error_handler.hpp](../../core/error_handler.md)
- [../../core/traits.hpp](../../core/traits.md)
- [../../core/types.hpp](../../core/types.md)
- [../abs.hpp](../abs.md)
- [../near_equal.hpp](../near_equal.md)
- [vector.hpp](vector.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename T, size_type Rows, size_type Columns> class matrix` | Fixed-size row-major matrix with contiguous inline storage. |
| `matrix()` / `explicit matrix(Args&&... args)` | Zero-initialize or construct from exactly `Rows * Columns` values in row-major order. |
| `rows()`, `columns()`, `size()`, `begin()`, `end()`, `data()` | Query dimensions and access contiguous storage. |
| `operator()(row, column)`, `at(row, column)` | Unchecked and checked element access; `at()` asserts on invalid indices. |
| Unary/binary arithmetic operators | Element-wise add, subtract, negate, and scale by a scalar. |
| `row(index)`, `column(index)` | Extract a row or column as a fixed-size vector. |
| `transpose()` / `transpose(value)` | Return the transposed matrix. |
| `trace()` | Sum the diagonal of a square matrix. |
| `operator+(...)`, `operator-(...)` | Free-function element-wise add/subtract with common-type promotion. |
| `operator*(matrix, matrix)` | Standard row-by-column matrix product with common-type promotion. |
| `operator*(matrix, vector)` | Multiply a row-major matrix by a column vector. |
| `hadamard(first, second)` | Element-wise matrix product. |
| `frobenius_squared(value)` | Sum of squares of all entries. |
| `diagonal_matrix(values)` | Build an `N x N` diagonal matrix from a vector. |
| `skew_symmetric(value)` | Return the 3x3 skew-symmetric matrix for a 3D vector. |
| `outer_product(first, second)` | Return the square outer-product matrix `first * second^T`. |
| `matrix2x2`, `matrix3x3`, `matrix4x4` | Convenience aliases for common square sizes. |
| `determinant_helper<T, N>` | Public recursive helper used by `determinant()`; mainly an implementation detail. |
| `determinant(value)` | Recursive cofactor-expansion determinant for square matrices. |
| `try_inverse(value, result, epsilon)` | Floating-point Gauss-Jordan inversion with partial pivoting; returns `false` for singular matrices. |
| `inverse(value, epsilon)` | Assert-on-failure wrapper around `try_inverse()`. |
| `identity<T, N>()` | Build an `N x N` identity matrix. |
| `near_equal(first, second, epsilon)` | Relative-tolerance comparison for floating-point matrices. |

## Usage Example
```cpp
#include "castle/math/linalg/matrix.hpp"

int main()
{
    const castle::math::matrix<float, 2U, 2U> value(1.0F, 2.0F, 3.0F, 4.0F);
    const castle::math::vector<float, 2U> input(1.0F, 0.0F);
    const castle::math::vector<float, 2U> result = value * input;
    (void)result;
}
```
See `samples/sample_matrix.cpp`.

## Constraints & Notes
- Storage is contiguous and row-major: element `(r, c)` resides at offset `r * Columns + c`.
- Matrix multiplication still follows the conventional algebraic `matrix * column_vector` model, so homogeneous translations occupy the final column.
- `determinant()` uses recursive cofactor expansion and is best suited to small fixed matrices.
- `try_inverse()` and `inverse()` are available only for floating-point element types and use an explicit `epsilon` pivot tolerance.
- `inverse()` asserts on singular matrices; call `try_inverse()` when failure must be handled explicitly.
