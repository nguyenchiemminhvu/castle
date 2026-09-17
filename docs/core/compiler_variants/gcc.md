# GCC compiler variant

## Overview

GCC-specific compiler feature definitions used by Castle portability macros.

## Header

```cpp
#include "castle/core/compiler_variants/gcc.h"
```

**Namespace:** `castle::detail`

## Main API

| API | Purpose |
|---|---|
| GCC feature macros | Definitions consumed by `core/compiler.h`. |

## Example

```cpp
// Included indirectly by castle/core/compiler.h.
```

## Embedded notes

- The branch targets GCC-style compiler builtins directly in low-level code.
