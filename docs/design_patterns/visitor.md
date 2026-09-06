# Visitor pattern

## Overview

Compile-time visitor helpers for dispatching behavior across a family of types.

## Header

```cpp
#include "castle/design_patterns/visitor.h"
```

**Namespace:** `castle::design_patterns`

## Main API

| API | Purpose |
|---|---|
| Visitor base/helper | Define the visitor interface expected by visitable types. |
| Accept/visit dispatch | Connect a concrete object to the correct visitor operation. |

## Example

```cpp
// Use the visitor helper with the concrete types supported by your model.
```

## Embedded notes

- The pattern avoids dynamic allocation; the exact dispatch model is defined by the templates in this header.
