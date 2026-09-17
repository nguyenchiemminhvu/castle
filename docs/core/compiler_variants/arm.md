# ARM Compiler Variant

## Overview
This backend header identifies ARM builds for Castle's compiler layer and reuses the GCC-style attribute/builtin mapping used by ARM toolchains that support it.

## Header
`#include "castle/core/compiler_variants/arm.hpp"`

## Dependencies
- [gcc.md](gcc.md)

## Public API
| API | Description |
|---|---|
| `CASTLE_COMPILER_ARM` | Indicates that the ARM backend header was selected. |
| GCC-style backend reuse | Pulls in the GCC variant definitions so `CASTLE_INLINE`, branch hints, packing attributes, and spin hints remain available. |

## Usage Example
```cpp
#include "castle/core/compiler_variants/arm.hpp"

static_assert(CASTLE_COMPILER_ARM == 1, "ARM backend header selected");
```

See also: `samples/sample_arm.cpp`.

## Constraints & Notes
- Usually included indirectly through `castle/core/compiler.hpp`.
- Does not add ARM-specific runtime behavior; it only layers identification over the GCC-style macro set.
