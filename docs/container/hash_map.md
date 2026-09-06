# Fixed-capacity hash map

## Overview

Associative key/value container built around Castle’s fixed-capacity hash table.

## Header

```cpp
#include "castle/container/hash_map.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `hash_map<Key, T, N, Hash, Equal>` | Key/value hash table with maximum capacity `N`. |
| Lookup | Find keys and retrieve mapped values. |
| Insertion | Insert or emplace key/value pairs. |
| `erase()` / `clear()` | Remove keys without reallocating. |
| Capacity queries | Inspect size, capacity, empty/full state. |

## Example

```cpp
castle::container::hash_map<int, int, 32> m;
m.emplace(10, 100);
auto p = m.get(10);
```

## Embedded notes

- Hash-table storage is fixed.
- Lookup cost depends on the supplied hash function and load factor.
