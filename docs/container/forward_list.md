# Fixed-capacity forward list

## Overview

A singly linked list whose nodes come from an internal fixed-capacity pool. No heap allocation is required.

## Header

```cpp
#include "castle/container/forward_list.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `forward_list<T, N>` | At most `N` live nodes. |
| Forward iterators | `before_begin`, `begin`, `end` and const forms. |
| Front operations | `push_front`, `emplace_front`, `pop_front`. |
| After-position operations | `emplace_after`, insertion/erase helpers around an iterator. |
| `find()` | Linear search for an equal value. |
| `clear()` | Destroy all active nodes and return them to the pool. |

## Example

```cpp
castle::container::forward_list<int, 8> list;
list.push_front(10);
list.push_front(20);
auto it = list.find(10);
```

## Embedded notes

- Iteration and search are O(N).
- Node addresses stay stable while the node is stored in the list.
