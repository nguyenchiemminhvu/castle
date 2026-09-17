# Polygon2D

## Overview
A fixed-capacity container of 2D vertices stored directly inside the object. Use it when polygon memory must be deterministic and heap-free.

## Header
`#include "castle/math/geometry/polygon2d.hpp"`

## Dependencies
- [`detail.md`](detail.md)
- [`point2d.md`](point2d.md)
- [`../../container/array.md`](../../container/array.md)
- [`../../core/compiler.md`](../../core/compiler.md)
- [`../../core/error_handler.md`](../../core/error_handler.md)
- [`../../core/traits.md`](../../core/traits.md)
- [`../../core/types.md`](../../core/types.md)
- [`../../error/status.md`](../../error/status.md)

## Public API
| API | Description |
|---|---|
| `polygon2d()` | Constructs an empty polygon. |
| `size()`, `capacity()`, `empty()`, `full()` | Inspect stored vertex count and fixed capacity. |
| `signed_area2()` | Returns twice the signed polygon area using widened intermediates where applicable. |
| `clockwise()` | Returns true when the stored vertex order has negative signed area. |
| `operator[](index)` | Indexed access to active vertices. |
| `begin()`, `end()`, `data()` | Access the active range stored in contiguous embedded memory. |
| `push_back(value)` | Appends a vertex or returns `status::full`. |
| `pop_back()` | Removes the last vertex or returns `status::empty`. |
| `clear()` | Removes all stored vertices. |

## Usage Example
See `samples/sample_polygon2d.cpp`.

```cpp
#include "castle/math/geometry/polygon2d.hpp"

castle::math::polygon2d<int, 4> triangle;
triangle.push_back(castle::math::point2d<int>(0, 0));
triangle.push_back(castle::math::point2d<int>(4, 0));
triangle.push_back(castle::math::point2d<int>(0, 3));
```

## Constraints & Notes
- Memory is embedded; no heap allocation occurs.
- The class does not enforce convexity, closure, or any specific winding order.
- `signed_area2()` returns twice the area and preserves sign to encode winding.
- Only the first `size()` entries in the underlying storage are logically part of the polygon.
