# ARM compiler variant

## Overview

ARM-specific compiler feature definitions included by the compiler abstraction layer.

## Header

```cpp
#include "castle/core/compiler_variants/arm.h"
```

**Namespace:** `castle::detail`

## Main API

| API | Purpose |
|---|---|
| ARM feature macros | Specialized definitions for ARM toolchains supported by Castle. |

## Example

```cpp
// Included indirectly by castle/core/compiler.h on an ARM build.
```

## Embedded notes

- Application code should normally use the public `CASTLE_*` macros instead of backend-specific ones.
