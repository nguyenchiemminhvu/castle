# Castle compile-time configuration

## Overview

Central configuration constants and options used by Castle components, especially fixed in-place storage defaults and error/callback behavior.

## Header

```cpp
#include "castle/core/config.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| Configuration constants | Defaults used by in-place storage and related facilities. |
| Feature switches | Compile-time options consumed by other Castle headers. |

## Example

```cpp
// Prefer project configuration before including headers that consume it.
#include "castle/core/config.h"
```

## Embedded notes

- Configuration is compile-time and does not require a runtime setup object.
