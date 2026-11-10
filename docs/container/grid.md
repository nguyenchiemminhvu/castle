# grid

## Overview
Fixed-size two-dimensional container with contiguous, row-major storage inside the object. Use it when a table has dimensions known at compile time, such as calibration maps, lookup tables, occupancy or tile maps, or image tiles. It is purely a data structure with no arithmetic. [`castle::math::matrix`](../math/linalg/matrix.md) stores its elements in a `grid` and adds linear algebra on top.

## Header
`#include "castle/container/grid.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [error_handler](../core/error_handler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)
- [array_view](array_view.md)
- [initializer_list](initializer_list.md)
- [reverse_iterator](../iterator/reverse_iterator.md)
- [forward](../utility/forward.md)

## Public API
| API | Description |
| --- | --- |
| `grid<T, Rows, Columns>` | Fixed-size 2D container holding `Rows * Columns` elements inline. Both dimensions must be positive. |
| `static_rows`, `static_columns`, `static_size` | Compile-time dimensions and element count. |
| `grid()` | Value-initializes every element. Arithmetic elements become zero. |
| `explicit grid(args...)` | Constructs from exactly `Rows * Columns` values in row-major order, each converted with `static_cast<T>`. `constexpr`. |
| `grid(initializer_list<T>)` | Constructs from a row-major brace list; asserts that its size equals `Rows * Columns`. |
| `rows()`, `columns()`, `size()`, `empty()` | Dimension queries. `empty()` is always `false`. |
| `begin()/end()/cbegin()/cend()` | Pointer iterators in row-major order. O(1). |
| `rbegin()/rend()/crbegin()/crend()` | Reverse iterators. O(1). |
| `operator()(row, column)` | Unchecked element access. O(1). |
| `at(row, column)` | Element access that asserts both indices are in range. O(1). |
| `row(index)` | `array_view<T>` (or `array_view<const T>`) over one contiguous row; asserts `index < Rows`. O(1). |
| `data()` | Pointer to the row-major storage. |
| `fill(value)` | Assigns `value` to every element. O(Rows * Columns). |
| `operator==`, `operator!=` | Element-wise comparison using `T::operator==`. O(Rows * Columns). |
| `row_view`, `const_row_view` | Aliases for `array_view<T>` and `array_view<const T>`. |

## Usage Example
```cpp
#include "castle/container/grid.hpp"

constexpr castle::container::grid<uint16_t, 2U, 3U> table(
    10U, 20U, 30U,
    40U, 50U, 60U);
static_assert(table(1U, 2U) == 60U, "row-major lookup");

castle::container::grid<uint8_t, 4U, 4U> map;
map.fill(0U);
map.at(2U, 3U) = 1U;
castle::container::array_view<uint8_t> third_row = map.row(2U);
```

See `samples/container/grid.cpp`. For matrix interop, see `samples/math/linalg/matrix.cpp`.

## Constraints & Notes
- No dynamic allocation. `sizeof(grid<T, R, C>) == sizeof(T) * R * C`.
- Element `(r, c)` resides at `data()[r * Columns + c]`. Rows are contiguous; columns are strided by `Columns`, so only rows are exposed as views.
- `T` must be default-constructible. Equality requires `T` to provide `operator==`.
- No arithmetic is defined. Use [`castle::math::matrix`](../math/linalg/matrix.md) for linear algebra.
- `operator()` is unchecked. `at()` and `row()` report out-of-range indices through `CASTLE_ASSERT`.
- Iterators, pointers, references, and row views stay valid until the grid is destroyed.
- Not thread-safe; concurrent mutation requires external synchronization.
