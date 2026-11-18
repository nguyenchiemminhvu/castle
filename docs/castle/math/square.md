# Square

## Overview
Tiny helper for expressing `value * value` directly in math-heavy code without changing the underlying arithmetic rules.

## Header
`#include "castle/math/square.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)

## Public API
| API | Description |
|---|---|
| `template <typename T> T castle::math::square(T value)` | Returns `value * value` in the same type `T`. |

## Usage Example
See `samples/sample_square.cpp` for a complete example.

```cpp
const int area_term = castle::math::square(12);
const float error_power = castle::math::square(1.5f);
```

## Constraints & Notes
- The result is not widened or saturated.
- Overflow and precision follow the normal rules of the chosen type.
- Complexity is constant time and the function is `constexpr`.
