# Observer pattern

## Overview

Compile-time-friendly observer interfaces for notifying registered dependents without requiring a standard-library container.

## Header

```cpp
#include "castle/design_patterns/observer.h"
```

**Namespace:** `castle::design_patterns`

## Main API

| API | Purpose |
|---|---|
| Observer types | Define an observer callback/interface for a subject. |
| Notify/attach/detach operations | Use the provided pattern API to propagate a state change. |

## Example

```cpp
// The exact observer types depend on the header’s template parameters.
// Keep the subject and observer lifetimes explicit in embedded code.
```

## Embedded notes

- The pattern is most useful when ownership is clear and bounded.
- Avoid keeping dangling observer pointers; Castle does not use GC-style lifetime management.
