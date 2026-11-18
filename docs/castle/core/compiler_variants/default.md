# Default Compiler Variant

## Overview
This fallback backend supplies conservative no-op or standard-C++ definitions when Castle does not recognize a dedicated compiler variant.

## Header
`#include "castle/core/compiler_variants/default.hpp"`

## Dependencies
None.

## Public API
| API | Description |
|---|---|
| `CASTLE_COMPILER_UNKNOWN` | Indicates that Castle is using the fallback backend. |
| `CASTLE_INLINE`, `CASTLE_HOT`, `CASTLE_COLD`, `CASTLE_NOINLINE` | Provide portable function annotation hooks, degrading to plain `inline` or empty definitions here. |
| `CASTLE_LIKELY(x)`, `CASTLE_UNLIKELY(x)` | Preserve the condition expression without backend-specific prediction hints. |
| `CASTLE_UNREACHABLE()`, `CASTLE_PACKED_ATTR`, `CASTLE_RESTRICT`, `CASTLE_CPU_RELAX()` | Conservative fallback definitions for unreachable-code hints, packing, restrict qualification, and spin hints. |

## Usage Example
```cpp
#include "castle/core/compiler_variants/default.hpp"

static_assert(CASTLE_COMPILER_UNKNOWN == 1, "fallback backend header selected");
```

See also: `samples/sample_default.cpp`.

## Constraints & Notes
- Usually included indirectly through `castle/core/compiler.hpp`.
- Designed for portability first: unsupported optimization hints degrade to harmless no-ops.
