# Fixed-capacity AVL tree

## Overview

A balanced binary search tree with parent links and preallocated node storage. Erase and rebalancing do not allocate.

## Header

```cpp
#include "castle/container/avl_tree.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `avl_tree<Key, T, N, Compare>` | Associative key/value tree with maximum `N` nodes. |
| Iterators | Bidirectional `iterator` and `const_iterator`. |
| Lookup | `find`, `lower_bound`, `upper_bound`, and related queries. |
| Insertion | Insert/emplace a key and mapped value while keeping AVL balance. |
| `erase(key)` | Remove an element and rebalance the tree. |
| Capacity/state | Size, empty/full, begin/end and clear operations. |

## Example

```cpp
castle::container::avl_tree<int, int, 16> tree;
tree.emplace(10, 100);
tree.emplace(5, 50);
auto it = tree.find(10);
```

## Embedded notes

- Node storage is fixed at compile time.
- Search is O(log N) for a balanced tree.
- Use the container when ordered lookup matters and a hash table is not a good fit.
