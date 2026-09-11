# Clang clock backend hook

## Overview

Clang compiler variant selected by Castle’s chrono clock dispatch.

## Header

```cpp
#include "castle/chrono/clock_variants/clang_clock_variant.h"
```

**Namespace:** `castle::chrono::detail`

## Main API

| API | Purpose |
|---|---|
| Clock variant | Backend selected for Clang builds. |

## Example

```cpp
// Included indirectly by castle/chrono/clocks.h on Clang.
```

## Embedded notes

- Keep application code against `castle::chrono::steady_clock` and `system_clock`.
