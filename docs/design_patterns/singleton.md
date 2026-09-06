# Singleton pattern

## Overview

Small singleton utility for objects that must have one process-wide instance without dynamic allocation.

## Header

```cpp
#include "castle/design_patterns/singleton.h"
```

**Namespace:** `castle::design_patterns`

## Main API

| API | Purpose |
|---|---|
| Singleton base/helper | Expose one static instance of the selected type. |
| Instance access | Use the pattern’s access function rather than allocating the object. |

## Example

```cpp
auto& service = castle::design_patterns::singleton<MyService>::instance();
```

## Embedded notes

- Use sparingly in embedded systems because global shared state can complicate testing and initialization.
