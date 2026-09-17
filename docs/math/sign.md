# Sign

## Overview
Small helper that reduces a value to `-1`, `0`, or `+1` for branch decisions and direction handling.

## Header
`#include "castle/math/sign.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)

## Public API
| API | Description |
|---|---|
| `template <typename T> int castle::math::sign(T value)` | Returns `-1` for negative values, `0` for zero, and `+1` for positive values. |

## Usage Example
See `samples/sample_sign.cpp` for a complete example.

```cpp
const int direction = castle::math::sign(-3);
const int enabled = castle::math::sign(1U);
```

## Constraints & Notes
- Unsigned inputs can only produce `0` or `+1`.
- The template assumes `value > T{0}` and `value < T{0}` are valid comparisons.
- Complexity is constant time and fully deterministic.
