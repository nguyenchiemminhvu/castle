# Containers

## Overview
Umbrella header that pulls the Castle container family into one include. Use it when a translation unit needs several containers and convenience matters more than minimizing header fan-in.

## Header
`#include "castle/container/containers.hpp"`

## Dependencies
- [array](array.md)
- [vector](vector.md)
- [ring_buffer](ring_buffer.md)
- [stack](stack.md)
- [string](string.md)
- [string_view](string_view.md)
- [array_view](array_view.md)
- [heap](heap.md)
- [avl_tree](avl_tree.md)
- [map](map.md)
- [set](set.md)
- [hash_table](hash_table.md)
- [hash_map](hash_map.md)
- [hash_set](hash_set.md)
- [forward_list](forward_list.md)

## Public API
| API | Description |
| --- | --- |
| `containers.hpp` | Defines no new types or functions; including it makes the listed container headers available. |

## Usage Example
See `samples/sample_containers.cpp`.

```cpp
#include "castle/container/containers.hpp"

castle::container::array<int, 2U> values{1, 2};
castle::container::map<int, int, 2U> ordered;
ordered.insert(1, 10);
```

## Constraints & Notes
- No allocation behavior is introduced by this header; it only aggregates other Castle headers.
- Thread-safety depends entirely on the specific container types you use; no synchronization is added here.
- Prefer individual headers when compile time or preprocessing footprint matters.
