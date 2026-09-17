# ARM clock backend hook

## Overview

ARM compiler variant selected by Castle’s chrono clock dispatch. The file supplies the platform-specific hook layer used by `clocks.h`.

## Header

```cpp
#include "castle/chrono/clock_variants/arm_clock_variant.h"
```

**Namespace:** `castle::chrono::detail`

## Main API

| API | Purpose |
|---|---|
| Clock variant | Backend selected when `CASTLE_COMPILER_ARM` is active. |

## Example

```cpp
// Included indirectly by castle/chrono/clocks.h on ARM.
```

## Embedded notes

- Applications normally use `clocks.h` instead of including this backend directly.
