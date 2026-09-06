# Fixed-capacity stack buffer

## Overview

A stack-like fixed-capacity buffer with deterministic storage. It supports LIFO operations and the same bounded style as the other Castle containers.

## Header

```cpp
#include "castle/container/stack.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| Stack type | Stores at most `N` values inside the object. |
| `push()` / `force_push()` | Add a value; the force form handles a full buffer according to the implementation policy. |
| `pop()` | Remove the most recently pushed value. |
| Top/state queries | Read the top value and inspect size/full/empty state. |
| Iterators/access | Use the provided indexed/iterator accessors where needed. |

## Example

```cpp
castle::container::stack<int, 8> s;
s.push(10);
s.push(20);
int x;
s.pop(x);
```

## Embedded notes

- Mutating operations report failure through Castle status/bool conventions instead of exceptions.
