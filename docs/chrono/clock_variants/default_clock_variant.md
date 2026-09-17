# Default clock backend hook

## Overview

Fallback clock backend used when no specialized compiler variant is selected.

## Header

```cpp
#include "castle/chrono/clock_variants/default_clock_variant.h"
```

**Namespace:** `castle::chrono::detail`

## Main API

| API | Purpose |
|---|---|
| Clock variant | Fallback implementation selected by the clock dispatcher. |

## Example

```cpp
// Included indirectly by castle/chrono/clocks.h.
```

## Embedded notes

- Use this only when integrating a compiler not covered by the dedicated variants.
