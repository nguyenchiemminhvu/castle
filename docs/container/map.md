# Ordered map wrapper

## Overview

Convenience associative-map type built on the fixed-capacity ordered tree implementation.

## Header

```cpp
#include "castle/container/map.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| Map type | Key/value mapping with fixed maximum size. |
| Lookup | `find`, `lower_bound`, `upper_bound` and related APIs. |
| Insertion | Insert/emplace entries. |
| Erase/clear | Remove entries and reset storage. |

## Example

```cpp
castle::container::map<int, int, 16> m;
m.emplace(1, 10);
auto it = m.find(1);
```

## Embedded notes

- Use an ordered map when key ordering is useful; use `hash_map` for hash-based lookup.
