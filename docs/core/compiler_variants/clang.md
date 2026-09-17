# Clang Compiler Variant

## Overview
This backend header identifies Clang builds for Castle's compiler layer and forwards to the GCC-style attribute/builtin mapping that Clang supports.

## Header
`#include "castle/core/compiler_variants/clang.hpp"`

## Dependencies
- [gcc.md](gcc.md)

## Public API
| API | Description |
|---|---|
| `CASTLE_COMPILER_CLANG` | Indicates that the Clang backend header was selected. |
| GCC-style backend reuse | Pulls in the GCC variant definitions so Castle can use the same low-level annotations and intrinsics under Clang. |

## Usage Example
```cpp
#include "castle/core/compiler_variants/clang.hpp"

static_assert(CASTLE_COMPILER_CLANG == 1, "Clang backend header selected");
```

See also: `samples/sample_clang.cpp`.

## Constraints & Notes
- Usually included indirectly through `castle/core/compiler.hpp`.
- Keeps Castle's portability layer small by sharing the GCC-style backend definitions.
