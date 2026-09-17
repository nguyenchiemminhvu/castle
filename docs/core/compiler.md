# Compiler abstraction layer

## Overview

Defines Castle compiler-detection macros and portability annotations so the rest of the library can keep one source code base across GCC, Clang, and ARM compiler variants.

## Header

```cpp
#include "castle/core/compiler.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| Compiler macros | Identify GCC, Clang, ARM, or fallback builds. |
| Function/type annotations | Macros such as `CASTLE_CONSTEXPR`, `CASTLE_NOEXCEPT`, `CASTLE_CONST`, `CASTLE_FINAL`, `CASTLE_DELETE`, and `CASTLE_DEFAULT`. |
| Feature macros | Conditional helpers such as `CASTLE_IF_CONSTEXPR` and `CASTLE_NODISCARD`. |

## Example

```cpp
CASTLE_CONSTEXPR int answer() CASTLE_NOEXCEPT { return 42; }
```

## Embedded notes

- Application code normally includes this header indirectly through other Castle headers.
- The abstraction is intentionally small to keep generated code predictable.
