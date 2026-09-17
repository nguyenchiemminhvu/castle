# Singleton

## Overview
An explicitly controlled singleton wrapper that constructs one instance in aligned static storage with no heap allocation. Use it when a process-wide object must be created and destroyed deterministically by the caller.

## Header
`#include "castle/design_patterns/singleton.hpp"`

## Dependencies
- [`../memory/alignment.md`](../memory/alignment.md)
- [`../memory/new.md`](../memory/new.md)
- `castle/core/error_handler.hpp`

## Public API
| API | Description |
| --- | --- |
| `singleton<T>` | Static-only wrapper around one `T` instance stored in aligned in-class storage. |
| `value_type` | Alias for the managed type `T`. |
| `create(Args&&...)` | Constructs the singleton in place; asserts if already created. |
| `destroy()` | Destroys the singleton instance; asserts if it does not exist. |
| `instance()` | Returns a reference to the constructed object; asserts if not created. |
| `is_valid()` | Reports whether the singleton currently contains a live object. |

## Usage Example
```cpp
// See: samples/sample_singleton.cpp
#include "castle/design_patterns/singleton.hpp"

struct service
{
    explicit service(uint32_t seed) : value(seed) {}
    uint32_t value;
};

using global_service = castle::design_patterns::singleton<service>;
```

## Constraints & Notes
- No heap allocation; construction uses placement new into aligned static storage.
- Callers control lifetime explicitly through `create()` and `destroy()`.
- Not thread-safe by itself; synchronize external access if multiple contexts may create or destroy the instance.
- Misuse is reported through Castle assertions instead of exceptions.
