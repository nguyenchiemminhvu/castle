# Hash

## Overview
This header provides deterministic hash functors for common embedded key types. It exists for fixed-capacity hash tables and registries that need stable hashing without the standard library.

## Header
`#include "castle/utility/hash.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [traits.md](../core/traits.md), [types.md](../core/types.md), [compare.md](compare.md), [string_view.md](../dsa/string_view.md)

## Public API
| API | Description |
|---|---|
| `hash_impl<T, ...>` | Helper selected internally by `hash<T>` for integral, enum, and pointer keys. |
| `hash<T>` | Default hash functor for integral, enum, and pointer types. |
| `hash<const T>` | Forwards hashing of const-qualified values to `hash<T>`. |
| `hash<dsa::basic_string_view<CharT>>` | Hashes every byte of a Castle string view. |
| `detail::double_hash_probe<Range>` | Internal enhanced double hashing sequence: turns one hash value into indexes in `[0, Range)` with one compare-and-subtract per step. Used by [`bloom_filter`](../dsa/bloom_filter.md) and [`count_min_sketch`](../dsa/count_min_sketch.md). Call `advance()` at most `Range` times. |
| `detail::hash_mix64(uint64_t)` | Internal 64-bit finalizer (SplitMix64) behind the integral hashes. [`cuckoo_filter`](../dsa/cuckoo_filter.md) uses it for fingerprints and alternate buckets; [`hyper_log_log`](../dsa/hyper_log_log.md) uses it to widen every key hash to 64 bits. |

## Usage Example
See `samples/sample_hash.cpp`.

```cpp
castle::size_type id_hash = castle::hash<uint32_t>()(123U);
castle::size_type name_hash = castle::hash<castle::dsa::string_view>()("TEMP");
```

## Constraints & Notes
- No dynamic allocation.
- Unsupported user-defined key types require an explicit specialization.
- String-view hashing processes the entire view contents; embedded nulls are hashed if present in the view length.
