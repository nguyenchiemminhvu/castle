# Error handling and assertions

## Overview

Provides the error/assertion policy used by Castle components that cannot return a status directly, such as `optional::value()` on an empty value.

## Header

```cpp
#include "castle/core/error_handler.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `CASTLE_ASSERT(condition, error)` | Route an invalid-state condition through the configured Castle error policy. |
| Generic error helpers | Create or pass Castle error information without exceptions. |
| Error hook/configuration | Connect the library to an application-specific assert, log, trap, or no-op policy. |

## Example

```cpp
// Application code typically just uses the Castle API that contains CASTLE_ASSERT.
castle::optional<int> v;
// v.value();  // handled by the configured error policy
```

## Embedded notes

- Castle does not require C++ exceptions for error reporting.
- Choose a policy that matches the safety level of the embedded product.
