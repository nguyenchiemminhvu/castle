# Fixed-capacity hash set

## Overview

A fixed-capacity set implemented with the Castle hash table machinery.

## Header

```cpp
#include "castle/container/hash_set.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `hash_set<T, N, Hash, Equal>` | Stores unique values in a fixed number of slots. |
| Insertion | Add a value if it is not already present. |
| Lookup | Find or test for a value. |
| `erase()` / `clear()` | Remove values without heap allocation. |
| Capacity/state | Size and fixed capacity queries. |

## Example

```cpp
castle::container::hash_set<int, 32> s;
s.insert(42);
if (s.contains(42)) { /* ... */ }
```

## Embedded notes

- Choose a capacity that matches the maximum live set size.
