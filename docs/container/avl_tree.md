# avl_tree

## Overview
`castle::container::avl_tree` is a fixed-capacity ordered associative tree that keeps itself height-balanced with AVL rotations. Use it when you need deterministic ordered lookup, lower/upper bound queries, and in-order iteration without heap allocation.

## Header
`#include "castle/container/avl_tree.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/types.md`](../core/types.md)
- [`../core/traits.md`](../core/traits.md)
- [`../iterator/tags.md`](../iterator/tags.md)
- [`../error/status.md`](../error/status.md)
- [`../memory/object.md`](../memory/object.md)
- [`../memory/construct.md`](../memory/construct.md)
- [`../memory/destroy.md`](../memory/destroy.md)
- [`../memory/static_storage.md`](../memory/static_storage.md)
- [`../utility/compare.md`](../utility/compare.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../utility/move.md`](../utility/move.md)
- [`../utility/pair.md`](../utility/pair.md)

## Public API
| API | Description |
|---|---|
| `avl_tree<Key, T, N, Compare>` | Fixed-capacity ordered key/value tree with room for at most `N` nodes. |
| Constructors | Default and comparator-taking constructors. |
| `capacity()`, `size()`, `available()`, `empty()`, `full()` | O(1) capacity/state queries. |
| `begin()/end()`, `cbegin()/cend()` | Bidirectional iterators over keys in sorted order. |
| `insert(key, value)` / `emplace(key, args...)` | O(log N) insertion; return `status::already_exists` or `status::full` on failure. |
| `find(key)`, `contains(key)` | O(log N) exact-key lookup. |
| `lower_bound(key)`, `upper_bound(key)` | O(log N) ordered bound queries. |
| `mapped(key)` | O(log N) pointer access to the mapped value or `nullptr`. |
| `erase(key)` | O(log N) erase by key; returns `status::not_found` when absent. |
| `erase(iterator)` | O(log N) erase by iterator and return the in-order successor. |
| `clear()` | O(size + N) destroy all nodes and rebuild the free-node pool. |

## Usage Example
See [`samples/sample_avl_tree.cpp`](../../samples/sample_avl_tree.cpp).

```cpp
castle::container::avl_tree<int, int, 8U> tree;
tree.emplace(20, 200);
tree.emplace(10, 100);
tree.emplace(30, 300);
CASTLE_SAMPLE_CHECK(tree.lower_bound(15)->first == 20);
```

## Constraints & Notes
- No dynamic allocation; all nodes are stored in an internal fixed-size pool.
- Lookup, insertion, erasure, and bound queries are O(log N) because AVL rebalancing keeps subtree heights within one.
- Successful insertion and erasure may perform single or double rotations while walking back toward the root.
- Node addresses stay stable while the node remains in the tree; rebalancing relinks nodes but does not move stored values.
- Iterators and references to an erased node become invalid; `clear()` invalidates all iterators.
