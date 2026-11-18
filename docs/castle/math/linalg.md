# Linear Algebra Umbrella

## Overview
This is the top-level Castle linear algebra umbrella header. Include it when a translation unit needs the full fixed-size vector, matrix, trigonometry, quaternion, and transform surface from one entry point.

## Header
`#include "castle/math/linalg.hpp"`

## Dependencies
- [math/linalg/linalg.hpp](linalg/linalg.md)

## Public API
| API | Description |
| --- | --- |
| `#include "castle/math/linalg.hpp"` | Re-exports the aggregated linear algebra module from `castle/math/linalg/linalg.hpp`. |

## Usage Example
```cpp
#include "castle/math/linalg.hpp"

int main()
{
    const castle::math::matrix<float, 3U, 3U> rotation =
        castle::math::rotation_2d_degrees(90.0F);
    const castle::math::vector<float, 3U> point(1.0F, 0.0F, 1.0F);
    const castle::math::vector<float, 3U> result = rotation * point;
    (void)result;
}
```
See `samples/sample_linalg.cpp`.

## Constraints & Notes
- This header adds no new runtime logic; it only includes the aggregated linear algebra header.
- The underlying components are fixed-size, allocation-free, exception-free, RTTI-free, virtual-free, and STL-free.
- Matrix storage is row-major, vectors are treated as columns, and trigonometric precision comes from the platform C math implementation wrapped by Castle.
