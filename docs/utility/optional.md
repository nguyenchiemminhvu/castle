# Optional value

## Overview

Heap-free optional value storage. It either contains one live `T` or is empty, and it explicitly manages the object lifetime in internal aligned storage.

## Header

```cpp
#include "castle/utility/optional.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `nullopt_t` / `nullopt` | Tag and value used to represent an empty optional. |
| `optional<T>` | Stores zero or one `T` without dynamic allocation. |
| `has_value()` / `operator bool` | Check whether a value is present. |
| `value()` / `operator*` / `operator->` | Access the stored value; invalid empty access is routed through Castle’s configured error policy. |
| `reset()` | Destroy the contained value and make the optional empty. |
| `emplace()` | Construct a new value in-place. |
| `value_or()` | Return the value or a supplied default. |
| Iterators | `begin/end` expose zero-or-one element behavior. |
| `swap()` | Exchange contained values. |

## Example

```cpp
castle::optional<int> v;
if (!v) { v.emplace(42); }
int x = v.value_or(0);
```

## Embedded notes

- Assignment reconstructs the contained object, so the type can be useful even when `T` is not assignable.
- No heap, RTTI, or exceptions are required.
