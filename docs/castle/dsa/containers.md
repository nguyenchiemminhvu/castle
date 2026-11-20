# Containers

## Overview

`castle::dsa::containers.hpp` is the umbrella include for CASTLE's fixed-capacity data structures, including the trie, Fenwick tree, segment trees, Bloom filter, count-min sketch, top-k summary, cuckoo filter, HyperLogLog, and t-digest.

## Header

`#include "castle/dsa/containers.hpp"`

## Dependencies

The umbrella header aggregates the DSA headers directly, among them [`trie`](trie.md), [`fenwick_tree`](fenwick_tree.md), [`segment_tree`](segment_tree.md), [`bloom_filter`](bloom_filter.md), [`count_min_sketch`](count_min_sketch.md), [`top_k`](top_k.md), [`cuckoo_filter`](cuckoo_filter.md), [`hyper_log_log`](hyper_log_log.md), and [`t_digest`](t_digest.md). Each component has its own page in this directory.

## Public API

| Header | Public component |
|---|---|
| `castle/dsa/trie.hpp` | `castle::dsa::trie`, `castle::dsa::contiguous_alphabet`, `lowercase_alphabet`, `digit_alphabet` |
| `castle/dsa/fenwick_tree.hpp` | `castle::dsa::fenwick_tree` |
| `castle/dsa/segment_tree.hpp` | `castle::dsa::segment_tree`, `castle::dsa::lazy_segment_tree`, monoids and lazy policies |
| `castle/dsa/bloom_filter.hpp` | `castle::dsa::bloom_filter`, `castle::dsa::bloom_filter_for`, `castle::dsa::bloom_optimal_hash_count` |
| `castle/dsa/count_min_sketch.hpp` | `castle::dsa::count_min_sketch`, `castle::dsa::count_min_sketch_for`, `castle::dsa::count_min_update`, `castle::dsa::count_min_width`, `castle::dsa::count_min_depth` |
| `castle/dsa/top_k.hpp` | `castle::dsa::top_k` |
| `castle/dsa/cuckoo_filter.hpp` | `castle::dsa::cuckoo_filter`, `castle::dsa::cuckoo_filter_for`, `castle::dsa::cuckoo_bucket_count` |
| `castle/dsa/hyper_log_log.hpp` | `castle::dsa::hyper_log_log` |
| `castle/dsa/t_digest.hpp` | `castle::dsa::t_digest` |
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

castle::dsa::bloom_filter_for<uint32_t, 16U> known_ids;
known_ids.insert(0x7DFU);

castle::dsa::count_min_sketch_for<uint32_t, 50U, 2U, uint16_t> frame_counts;
frame_counts.add(0x7DFU);

castle::dsa::top_k<uint32_t, 8U> top_talkers;
top_talkers.add(0x7DFU);

castle::dsa::cuckoo_filter_for<uint32_t, 32U> active_sessions;
active_sessions.insert(0x7DFU);
active_sessions.erase(0x7DFU);

castle::dsa::hyper_log_log<uint32_t, 8U> distinct_ids;
distinct_ids.add(0x7DFU);

castle::dsa::t_digest<float, 32U> latency_ms;
latency_ms.add(1.5F);
```

See [`samples/castle/dsa/containers.cpp`](../../samples/dsa/containers.cpp) for a standalone sample.

## Constraints & Notes

- This header introduces no storage or runtime behavior of its own.
- Allocation, capacity, operation, and thread-safety constraints are defined by each included component.
- Prefer individual headers when compile time or preprocessor footprint matters.
