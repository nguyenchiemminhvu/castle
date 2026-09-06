# Object lifetime utilities

## Overview

Low-level helpers for starting and ending object lifetimes in caller-managed storage.

## Header

```cpp
#include "castle/memory/lifetime.h"
```

**Namespace:** `castle::memory`

## Main API

| API | Purpose |
|---|---|
| Lifetime helpers | Utilities that test or manage whether a raw storage region contains a live object. |
| Construction/destruction glue | Helpers used by container storage implementations. |

## Example

```cpp
// Usually used internally by fixed-capacity containers and storage types.
```

## Embedded notes

- Lifetime code should be kept localized and well tested.
