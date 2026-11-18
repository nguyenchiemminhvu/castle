# Containers

## Overview

`castle::dsa::containers.hpp` is the umbrella include for CASTLE's fixed-capacity data structures, including the trie, Fenwick tree, and segment trees.

## Header

`#include "castle/dsa/containers.hpp"`

## Dependencies

The umbrella header aggregates the DSA headers directly, among them [`trie`](trie.md), [`fenwick_tree`](fenwick_tree.md), and [`segment_tree`](segment_tree.md). Each component has its own page in this directory.

## Public API

| Header | Public component |
|---|---|
| `castle/dsa/trie.hpp` | `castle::dsa::trie`, `castle::dsa::contiguous_alphabet`, `lowercase_alphabet`, `digit_alphabet` |
| `castle/dsa/fenwick_tree.hpp` | `castle::dsa::fenwick_tree` |
| `castle/dsa/segment_tree.hpp` | `castle::dsa::segment_tree`, `castle::dsa::lazy_segment_tree`, monoids and lazy policies |
| Other headers listed in `containers.hpp` | Existing CASTLE container types. |

This header declares no additional classes or functions; including it makes the component headers available.

## Usage Example

```cpp
#include "castle/dsa/containers.hpp"

castle::dsa::trie<castle::dsa::lowercase_alphabet, 8U, 16U> dictionary;
dictionary.insert("can");

const int32_t values[] = {2, 3, 5};
castle::dsa::fenwick_tree<int32_t, 4U> prefix_sums;
prefix_sums.build(values, 3U);

castle::dsa::segment_tree<castle::dsa::sum_monoid<int32_t>, 4U> interval_sums;
interval_sums.build(values, 3U);
```

See [`samples/castle/dsa/containers.cpp`](../../samples/dsa/containers.cpp) for a standalone sample.

## Constraints & Notes

- This header introduces no storage or runtime behavior of its own.
- Allocation, capacity, operation, and thread-safety constraints are defined by each included component.
- Prefer individual headers when compile time or preprocessor footprint matters.
