# top_k

## Overview
Fixed-size top-k summary based on the **Space-Saving** algorithm. It tracks the most frequent keys of a stream in `K` slots, however many distinct keys the stream contains. Each tracked key has a `count` and an `error`:

- `count - error <= true count <= count` for every tracked key (deterministic, not probabilistic);
- every untracked key has a true count of at most `min_count()`, and `min_count() <= N / K` for a stream total `N`;
- so every key that makes up more than `1 / K` of the stream is **always** tracked.

A new key always enters. When all slots are used, it replaces the key with the smallest count and inherits that count as its error. Space-Saving trades exact counts for a fixed, small memory: it overestimates light keys, never heavy ones. Typical embedded uses:

- Top talkers on a CAN bus, an Ethernet port, or a UART multiplexer.
- Most frequent DTCs, error codes, or log events per drive cycle.
- Hot flash pages or EEPROM records for wear leveling or caching.
- Busiest interrupt sources or most requested resources.

Compared with [`count_min_sketch`](count_min_sketch.md): the sketch estimates the count of **any** key but cannot list keys. `top_k` lists the heaviest keys and their counts, and gives deterministic bounds in less memory.

Theory: [Metwally, Agrawal, El Abbadi: Efficient Computation of Frequent and Top-k Elements in Data Streams](https://www.cs.ucsb.edu/sites/default/files/documents/2005-23.pdf), [Streaming algorithms: frequent elements (Wikipedia)](https://en.wikipedia.org/wiki/Streaming_algorithm#Frequent_elements).

## Header
`#include "castle/dsa/top_k.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md) (`meta::is_integral`, `meta::is_unsigned`, `meta::is_same`)
- [types](../core/types.md)
- [algorithm](algorithm.md) (`castle::sort`, `detail::heap_sift_down`)
- [array](array.md) (slot storage)
- [array_view](array_view.md) (`top()` output)
- [saturating](../math/saturating.md) (`math::saturating_add`)
- [compare](../utility/compare.md) (`castle::equal_to`)

## Public API
### `top_k<Key, K, Counter = uint32_t, KeyEqual = castle::equal_to<Key>>`
| API | Description |
| --- | --- |
| `top_k<Key, K, Counter, KeyEqual>` | `K` slots. Requires `K > 0` and an unsigned integral `Counter` (not `bool`). `Key` must be default-constructible and copy-assignable. `KeyEqual` compares keys from a `const` `operator()`. |
| `key_type`, `counter_type`, `key_equal`, `size_type`, `const_iterator` | `Key`, `Counter`, `KeyEqual`, `castle::size_type`, `const entry_type*`. |
| `entry_type` | `{ key, count, error }`. `count` is an upper bound of the true count; `error` is the largest possible overestimate. |
| `counter_type entry_type::guaranteed() const` | Lower bound `count - error` of the true count. |
| `top_k()` | Empty summary with a default-constructed `KeyEqual`. |
| `explicit top_k(const KeyEqual& equal)` | Empty summary with a caller-supplied key equality. |
| `static constexpr size_type capacity()` | `K`. |
| `size_type size() const` | Number of tracked keys, `0 .. K`. |
| `bool empty() const`, `bool full() const` | `size() == 0`, `size() == K`. When full, the next new key evicts the minimum. |
| `uint64_t total() const` | Sum `N` of every added count, saturated at `UINT64_MAX`. |
| `counter_type min_count() const` | Smallest tracked count when full, else 0. Upper bound of the true count of every untracked key. O(1). |
| `counter_type add(const Key& key, Counter count = 1)` | Adds `count` occurrences and returns `estimate(key)` afterwards. A new key takes a free slot or replaces the minimum. `count == 0` changes nothing. Counts saturate. O(K). |
| `counter_type estimate(const Key& key) const` | Tracked count, or `min_count()` for an untracked key. Never below the true count while nothing saturates. O(K). |
| `const entry_type* find(const Key& key) const` | Entry of a tracked key, or `nullptr`. Valid until the next mutation. O(K). |
| `bool contains(const Key& key) const` | `true` when `key` is tracked. O(K). |
| `size_type top(array_view<entry_type> out)` | Writes the `min(out.size(), size())` heaviest entries, highest count first, and returns how many were written. Reorders the internal storage, so it invalidates `find()` pointers and iterators. O(K log K). |
| `void halve()` | Halves every count, rounding down, and recomputes each error so `count - error <= floor(f / 2) <= count`. Halves `total()`. O(K). |
| `void clear()` | Forgets every key and the total. O(1). |
| `const_iterator begin() const`, `end() const` | Tracked entries in internal (heap) order. |
| `const key_equal& key_eq() const` | Stored key equality functor. |

## Usage Example
```cpp
#include "castle/dsa/top_k.hpp"

using talkers_type = castle::dsa::top_k<uint32_t, 8U, uint16_t>; // 8 slots, 64 bytes
talkers_type talkers;

talkers.add(frame_id); // for every received frame

talkers_type::entry_type busiest[3];
const castle::size_type n = talkers.top(castle::dsa::array_view<talkers_type::entry_type>(busiest, 3U));
for (castle::size_type i = 0U; i < n; ++i)
{
    if (busiest[i].guaranteed() > talkers.min_count())
    {
        // busiest[i].key is certainly heavier than every key that is not tracked.
    }
}
```

`samples/castle/dsa/top_k.cpp` finds the two busiest identifiers on a simulated CAN bus with 52 distinct ids in 4 slots. It also ranks fault codes per drive cycle with `string_view` keys and `halve()`.

## Constraints & Notes
- No allocation, exceptions, RTTI, or virtual dispatch. The object is one `dsa::array<entry_type, K>`, one size, one `uint64_t` total, and one `KeyEqual` object. The summary is copyable.
- **Error bound.** With `N = total()`, every count is at most `N / K` above the truth. Pick `K` from the smallest share of traffic you must catch:

  | `K` | Always-tracked share | Max overestimate | RAM (`uint32_t` key and counter) |
  | --- | --- | --- | --- |
  | 4 | > 25 % | `N / 4` | 48 B |
  | 8 | > 12.5 % | `N / 8` | 96 B |
  | 16 | > 6.25 % | `N / 16` | 192 B |
  | 64 | > 1.6 % | `N / 64` | 768 B |

  On skewed streams the real error is usually far below the bound. Check `entry.error` or `guaranteed()` per entry.
- **Certain ranking.** An entry with `guaranteed() > min_count()` outranks every untracked key. Two entries are ordered for certain when the `guaranteed()` of one exceeds the `count` of the other.
- **Complexity.** The slots form a min-heap on the count, so eviction and count updates cost O(log K). Lookup is a linear scan over the `K` keys: no hash, no index, and contiguous memory. That is the fastest choice for the small `K` used on microcontrollers; it is linear in `K` for large `K`.
- **Fill phase.** Until `K` keys arrive, every count is exact (`error == 0`) and `min_count()` is 0. The heap is built once, when the last slot fills.
- **Saturation.** Counts saturate at the `Counter` maximum and `total()` at `UINT64_MAX`. Saturated counts lose the bounds above. Use a wider `Counter` or call `halve()` periodically.
- **Key lifetime.** Tracked keys are copies. View keys such as `string_view` must outlive the summary.
- **No merge, no removal.** Space-Saving summaries can only be combined with extra temporary storage, which this component avoids. Use `clear()` or `halve()` to forget.
- Integer-only. Not thread-safe. Concurrent `const` calls are safe only when no thread writes.
