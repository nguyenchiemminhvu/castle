# Object construction helpers

## Overview

Constructs objects in caller-provided storage with placement construction, without allocating memory.

## Header

```cpp
#include "castle/memory/construct.h"
```

**Namespace:** `castle::memory`

## Main API

| API | Purpose |
|---|---|
| `construct_at<T>(address, args...)` | Construct `T` at the supplied address. |
| Forwarding construction | Preserves constructor arguments using Castle forwarding helpers. |

## Example

```cpp
T* p = storage.address(0);
castle::memory::construct_at<T>(p, 1, 2);
```

## Embedded notes

- The address must point to suitably aligned raw storage.
- After construction, the object’s lifetime is active and must eventually be destroyed.
