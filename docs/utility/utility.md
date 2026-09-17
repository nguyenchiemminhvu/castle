# Utility umbrella header

## Overview

Convenience header for Castle’s common move/forward/swap, pair/tuple, optional/variant and related utilities.

## Header

```cpp
#include "castle/utility/utility.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| Utility family | Includes the main generic utility primitives in one header. |

## Example

```cpp
#include "castle/utility/utility.h"
castle::optional<int> value;
```

## Embedded notes

- Use feature-specific headers to keep dependencies smaller when needed.
