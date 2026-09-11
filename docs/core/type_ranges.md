# Type range metadata

## Overview

Compile-time numeric range information used by safe integer conversions and other low-level code.

## Header

```cpp
#include "castle/core/type_ranges.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| Numeric range helpers | Expose minimum, maximum, and related bounds for supported types. |
| Range predicates | Support checking whether a value or type range fits into another representation. |

## Example

```cpp
// Range traits are mainly consumed by Castle conversion and math utilities.
```

## Embedded notes

- Use these helpers to make narrowing conversions explicit and deterministic.
