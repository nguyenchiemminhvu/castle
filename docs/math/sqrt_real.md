# Sqrt Real

## Overview
Floating-point square root helper for non-negative inputs, with deterministic behavior and no direct caller dependency on `<math.h>`.

## Header
`#include "castle/math/sqrt_real.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [error_handler](../core/error_handler.md)
- [traits](../core/traits.md)

## Public API
| API | Description |
|---|---|
| `template <typename T> T castle::math::sqrt_real(T value)` | Returns the square root of a non-negative floating-point value. |

## Usage Example
See `samples/sample_sqrt_real.cpp` for a complete example.

```cpp
const float distance = castle::math::sqrt_real(2.25f);
const double root_two = castle::math::sqrt_real(2.0);
```

## Constraints & Notes
- Negative inputs violate the precondition and trigger assertion handling.
- GCC/Clang builds use builtin square-root operations when available; other builds use deterministic range reduction plus bounded Newton iteration.
- Complexity is constant time with a fixed iteration budget in the fallback path.
