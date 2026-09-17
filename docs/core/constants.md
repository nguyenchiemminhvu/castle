# Core Constants

## Overview
This header collects constexpr math constants, angle-conversion helpers, character-encoding boundary values, and serializer limits that Castle components share.

## Header
`#include "castle/core/constants.hpp"`

## Dependencies
- [compiler.md](compiler.md)
- [types.md](types.md)
- [type_ranges.md](type_ranges.md)
- [traits.md](traits.md)

## Public API
| API | Description |
|---|---|
| `castle::math::pi<T>()`, `tau<T>()`, `e<T>()`, `golden_ratio<T>()`, `log2_e<T>()`, `log10_e<T>()`, `ln_2<T>()`, `ln_10<T>()`, `sqrt_2<T>()`, `sqrt_3<T>()`, `inv_sqrt_2<T>()` | Return common mathematical constants in a caller-selected floating-point type. |
| `castle::math::degrees_to_radians(T)`, `castle::math::radians_to_degrees(T)` | Convert angles between degrees and radians with `constexpr` arithmetic. |
| `castle::characters::*` | ASCII, UTF-8, and UTF-16 boundary constants used by text and parser code. |
| `castle::serialization::*` | String-length limits, Unicode bounds, and XML character-range constants used by serializers. |

## Usage Example
```cpp
#include "castle/core/constants.hpp"

constexpr double half_turn = castle::math::degrees_to_radians(180.0);
```

See also: `samples/sample_constants.cpp`.

## Constraints & Notes
- Math helpers participate only for floating-point `T` via Castle traits.
- All values are compile-time constants and require no runtime state.
- Character and serialization constants are raw protocol/encoding limits; they do not perform validation by themselves.
