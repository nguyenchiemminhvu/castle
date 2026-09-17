# Status codes

## Overview

Defines the small status type used by Castle APIs that report expected runtime failures without exceptions.

## Header

```cpp
#include "castle/error/status.h"
```

**Namespace:** `castle::status and castle::error`

## Main API

| API | Purpose |
|---|---|
| `status` | Enum-like result used by fixed-capacity containers and callback registries. |
| Common results | Includes states such as `ok`, `full`, `empty`, `not_found`, `out_of_range`, `invalid_argument`, `invalid_callback`, and `invalid_subscription` in the current branch. |

## Example

```cpp
auto rc = vector.push_back(10);
if (rc != castle::status::ok) { /* handle bounded failure */ }
```

## Embedded notes

- Treat status results as part of normal embedded control flow.
- Do not rely on exceptions being available.
