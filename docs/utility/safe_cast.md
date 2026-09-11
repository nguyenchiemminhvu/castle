# Checked numeric casts

## Overview

Conversion helpers intended to make narrowing and representation-changing integer conversions explicit and range-aware.

## Header

```cpp
#include "castle/utility/safe_cast.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| Safe conversion helpers | Check source/destination range before converting where supported. |
| Range traits | Use Castle type range metadata to keep checks compile-time friendly. |
| Failure reporting | Return a status/boolean-style result rather than throwing. |

## Example

```cpp
// Use the safe-cast API when converting protocol fields or counters
// into narrower target types.
```

## Embedded notes

- Prefer explicit checked conversion at hardware/protocol boundaries.
- For performance-critical code, understand whether a particular overload performs runtime checks or only compile-time validation.
