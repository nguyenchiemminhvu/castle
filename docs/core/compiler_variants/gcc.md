# GCC Compiler Variant

## Overview
This backend header provides GCC-style attributes, branch hints, layout attributes, and spin-wait hints used by Castle's portability layer.

## Header
`#include "castle/core/compiler_variants/gcc.hpp"`

## Dependencies
None.

## Public API
| API | Description |
|---|---|
| `CASTLE_COMPILER_GCC` | Indicates that GCC-style backend definitions are available. |
| `CASTLE_INLINE`, `CASTLE_HOT`, `CASTLE_COLD`, `CASTLE_NOINLINE` | Map Castle's function annotation hooks to GCC-style attributes. |
| `CASTLE_LIKELY(x)`, `CASTLE_UNLIKELY(x)` | Apply GCC branch-prediction hints to a condition expression. |
| `CASTLE_UNREACHABLE()` | Marks a code path as unreachable for optimization purposes. |
| `CASTLE_PACKED_ATTR`, `CASTLE_RESTRICT` | Expose GCC-style packed-layout and restrict qualifiers. |
| `CASTLE_CPU_RELAX()` | Emits the best-known spin-wait hint for the active architecture. |

## Usage Example
```cpp
#include "castle/core/compiler_variants/gcc.hpp"

CASTLE_INLINE int fast_path()
{
    return 1;
}
```

See also: `samples/sample_gcc.cpp`.

## Constraints & Notes
- Usually included indirectly through `castle/core/compiler.hpp`.
- `CASTLE_CPU_RELAX()` is architecture-dependent and falls back to a compiler barrier when no dedicated hint is known.
