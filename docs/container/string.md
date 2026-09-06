# Fixed-capacity string

## Overview

A null-terminated string with a compile-time maximum character capacity. Mutating operations return Castle status values rather than growing a heap buffer.

## Header

```cpp
#include "castle/container/string.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `basic_string<CharT, N>` | String with room for at most `N` characters plus terminator. |
| `assign()` | Assign from C strings or string views. |
| `append()` / insert helpers | Add characters while respecting capacity. |
| `erase()` / `resize()` | Remove or resize content within the fixed capacity. |
| Element/iterators | `operator[]`, `front`, `back`, `data`, `begin/end`. |
| `size()` / `capacity()` / `empty()` / `full()` | String state. |

## Example

```cpp
castle::container::string<32> name;
name.assign("sensor");
name.append("_01");
```

## Embedded notes

- A successful string always stays null-terminated.
- Capacity is the maximum logical character count, not the size of an external allocation.
