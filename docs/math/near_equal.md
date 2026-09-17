# Near Equal

## Overview
Caller-tuned floating-point comparison that combines absolute and relative tolerance in one deterministic check.

## Header
`#include "castle/math/near_equal.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)
- [abs](abs.md)
- [algorithm](../algorithm/algorithm.md)

## Public API
| API | Description |
|---|---|
| `template <typename T> bool castle::math::near_equal(T a, T b, T epsilon)` | Returns `true` when `abs(a - b) <= epsilon * max(1, abs(a), abs(b))`. |

## Usage Example
See `samples/sample_near_equal.cpp` for a complete example.

```cpp
const bool same = castle::math::near_equal(1000.0f, 1000.5f, 0.001f);
const bool zeroish = castle::math::near_equal(0.0004f, 0.0f, 0.001f);
```

## Constraints & Notes
- The scaling term is clamped to at least `1`, so values near zero use an absolute tolerance.
- Pass `epsilon` explicitly; the function does not choose one for you.
- `epsilon` should be non-negative. A negative tolerance makes the comparison fail for finite inputs.
