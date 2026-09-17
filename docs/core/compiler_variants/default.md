# Default compiler variant

## Overview

Fallback compiler definitions for toolchains without a dedicated Castle compiler variant.

## Header

```cpp
#include "castle/core/compiler_variants/default.h"
```

**Namespace:** `castle::detail`

## Main API

| API | Purpose |
|---|---|
| Fallback macros | Conservative definitions used by the portability layer. |

## Example

```cpp
// Included indirectly by castle/core/compiler.h.
```

## Embedded notes

- Add a dedicated variant when a new compiler needs stronger feature mappings.
