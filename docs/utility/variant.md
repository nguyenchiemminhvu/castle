# Type-safe variant

## Overview

Heap-free tagged union for holding one value from a fixed set of alternative types.

## Header

```cpp
#include "castle/utility/variant.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `variant<T...>` | Stores exactly one active alternative inside inline storage. |
| `index()` | Identify the active alternative by position. |
| `get<I>()` / typed access | Access the selected alternative. |
| `get_if()` | Return a pointer when the requested alternative is active. |
| `holds_alternative()` | Check which type is currently stored. |
| `visit()` | Dispatch a callable based on the active alternative. |
| Emplace/reset-style operations | Replace the active alternative using in-place construction. |

## Example

```cpp
castle::variant<int, float> v(10);
if (castle::holds_alternative<int>(v)) {
    auto x = castle::get<int>(v);
}
```

## Embedded notes

- Storage is fixed by the largest alternative; there is no heap allocation.
- Design around the actual alternative list so the object size remains predictable.
