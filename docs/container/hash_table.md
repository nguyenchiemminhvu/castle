# Fixed-capacity hash table core

## Overview

Low-level open-addressing hash table storage used by Castle hash containers. It keeps entry storage inside the table object.

## Header

```cpp
#include "castle/container/hash_table.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `hash_table<Key, T, N, Hash, Equal>` | Fixed-capacity table of keys and mapped values. |
| `capacity()` / `size()` | Table sizing information. |
| Lookup/get | Find an entry or get a mapped value pointer. |
| Insertion/emplace | Construct an entry in an empty/deleted slot. |
| `erase()` | Mark a slot deleted and destroy the stored entry. |
| `clear()` | Destroy all occupied entries and reset the table. |

## Example

```cpp
// Most application code should use hash_map/hash_set.
castle::container::hash_table<int, int, 16> table;
```

## Embedded notes

- The implementation uses empty/occupied/deleted slot states.
- The table is non-copyable and non-movable to keep storage identity simple.
