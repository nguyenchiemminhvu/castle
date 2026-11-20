# count_min_sketch

## Overview
Fixed-size count-min sketch: a probabilistic frequency table that stores counters, not keys. It answers "how often was this key added?" with an estimate that is:

- **never below** the true count (no undercount), unless that count exceeds the counter maximum;
- above the true count only by collisions with other keys, and by at most `epsilon * N` with probability `1 - delta`.

The sketch holds `Depth` rows of `Width` counters. Each key owns one counter per row. `add()` raises those counters; `estimate()` returns the smallest of them. Memory stays fixed however many distinct keys the stream has. Keys cannot be listed. Typical embedded uses:

- Find a babbling CAN node or a noisy interrupt source without one counter per identifier.
- Count DTC, error, or event occurrences per code on a small node.
- Rate-limit requests per client address over time windows (`halve()` ages old traffic).
- Pick hot flash pages, cache admission (TinyLFU), or top talkers on a gateway.
- Combine statistics from several nodes with `merge()`.

Theory: [Count-min sketch (Wikipedia)](https://en.wikipedia.org/wiki/Count%E2%80%93min_sketch), [Cormode: Count-Min Sketch](http://dimacs.rutgers.edu/~graham/pubs/papers/cmencyc.pdf).

## Header
`#include "castle/dsa/count_min_sketch.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [status](../core/status.md) (`castle::status` for `load_counters()`)
- [traits](../core/traits.md) (`meta::is_integral`, `meta::is_unsigned`, `meta::is_same`)
- [type_ranges](../core/type_ranges.md) (`castle::numeric_limits`)
- [types](../core/types.md)
- [grid](grid.md) (counter storage)
- [powi](../math/powi.md) (`math::powi`)
- [saturating](../math/saturating.md) (`math::saturating_add`)
- [hash](../utility/hash.md) (`castle::hash`, `detail::double_hash_probe`)

## Public API
### `count_min_update`
| Value | Description |
| --- | --- |
| `standard` | Adds the count to every row counter of the key. Sketches are exactly linear: `a.merge(b)` equals one sketch fed with both streams. |
| `conservative` | Raises each row counter only up to `estimate + count`. Same memory, smaller overestimates. `merge()` still never undercounts, but is no longer exactly linear. |

### Sizing functions
| API | Description |
| --- | --- |
| `constexpr size_type count_min_width(size_type inverse_epsilon)` | Returns `ceil(e * inverse_epsilon)`, at least 1. The estimate error is then at most `N / inverse_epsilon`. Integer-only. |
| `constexpr size_type count_min_depth(size_type confidence_nines)` | Returns `ceil(k * ln 10)`, at least 1, for the confidence `1 - 10^(-k)`. `k = 2` (99 %) gives 5 rows. Integer-only. |

### `count_min_sketch<Key, Width, Depth, Counter = uint32_t, Update = count_min_update::standard, Hash = castle::hash<Key>>`
| API | Description |
| --- | --- |
| `count_min_sketch<...>` | `Depth` rows of `Width` counters of type `Counter`. Requires `Width > 0`, `1 <= Depth <= Width`, `Width * Depth <= size_type(-1) / 2`, and an unsigned integral `Counter` (not `bool`). `Hash` returns `size_type` from a `const` `operator()`. |
| `key_type`, `counter_type`, `hasher`, `size_type`, `grid_type` | `Key`, `Counter`, `Hash`, `castle::size_type`, `grid<Counter, Depth, Width>`. |
| `static constexpr count_min_update update_rule` | `Update`. |
| `count_min_sketch()` | Empty sketch with a default-constructed `Hash`. |
| `explicit count_min_sketch(const Hash& hash)` | Empty sketch with a caller-supplied hasher, for example a seeded one. |
| `static constexpr size_type width()` | `Width`. |
| `static constexpr size_type depth()` | `Depth`. |
| `static constexpr size_type counter_count()` | `Width * Depth`. |
| `static constexpr Counter max_count()` | Saturation value of one counter. |
| `static constexpr float epsilon()` | Relative error `e / Width`. |
| `static constexpr float delta()` | Failure probability `e^(-Depth)` of the error bound. |
| `Counter add(const Key& key, Counter count = 1)` | Adds `count` occurrences of `key` and returns the new `estimate(key)`. Counters and `total()` saturate. O(Depth). |
| `Counter estimate(const Key& key) const` | Smallest row counter of `key`. Zero means `key` was certainly never added. O(Depth). |
| `void merge(const count_min_sketch& other)` | Adds every counter and the total of `other`, saturating. O(Width * Depth). |
| `void halve()` | Halves every counter and the total, rounding down. Ages old counts. O(Width * Depth). |
| `status load_counters(const Counter* counters, size_type count, uint64_t total)` | Restores `counter_count()` counters and the total. Returns `status::invalid_argument` if `counters == nullptr`, `status::out_of_range` if `count != counter_count()`, or `status::ok`. The sketch is unchanged on failure. |
| `void clear()` | Sets every counter and the total to zero. |
| `bool empty() const` | `true` when `total() == 0`. |
| `uint64_t total() const` | Sum `N` of every added count, saturated at `UINT64_MAX`. |
| `float error_bound() const` | `epsilon() * total()`: `estimate(x) <= f(x) + error_bound()` with probability `1 - delta()`. |
| `const grid_type& counters() const` | Read-only counters. `counters().data()` exposes them in row-major order for persistence or transfer. |
| `const hasher& hash_function() const` | Stored hash functor, for example to check seeds before `merge()`. |
| `operator==`, `operator!=` | Compares counters and totals. Hash functors are not compared. |

### `count_min_sketch_for<Key, InverseEpsilon, ConfidenceNines = 2, Counter = uint32_t, Update = count_min_update::standard, Hash = castle::hash<Key>>`
| API | Description |
| --- | --- |
| `count_min_sketch_for<...>` | Alias of `count_min_sketch<Key, count_min_width(InverseEpsilon), count_min_depth(ConfidenceNines), Counter, Update, Hash>`. Sizes the sketch from the error and confidence targets. |

## Usage Example
```cpp
#include "castle/dsa/count_min_sketch.hpp"

// Error <= 1 / 50 of all frames, 99 % confidence: 136 x 5 counters of 16 bits (1360 bytes).
castle::dsa::count_min_sketch_for<uint32_t, 50U, 2U, uint16_t> frames;

if (frames.add(frame_id) > babble_limit)
{
    // This identifier sends far too often: report the node.
}

// Explicit size, conservative update, aging per time window.
castle::dsa::count_min_sketch<uint32_t, 64U, 4U, uint8_t,
                              castle::dsa::count_min_update::conservative> requests;
requests.add(client_address);
requests.halve(); // at the end of each window
```

`samples/castle/dsa/count_min_sketch.cpp` detects a babbling CAN identifier among normal ones, rate-limits a client over time windows with `halve()`, and merges event counts from two ECUs keyed by `string_view`.

## Constraints & Notes
- No allocation, exceptions, RTTI, or virtual dispatch. The object is one `grid<Counter, Depth, Width>` (`Width * Depth * sizeof(Counter)` bytes), one `uint64_t` total, and one `Hash` object. The sketch is copyable.
- **Error guarantee.** With `N = total()`, `epsilon = e / Width`, and `delta = e^(-Depth)`: `f(x) <= estimate(x)` always, and `estimate(x) <= f(x) + epsilon * N` with probability at least `1 - delta`. Width sets the error; depth sets the confidence. Sizes from `count_min_sketch_for` (`uint16_t` counters):

  | `InverseEpsilon` | `ConfidenceNines` | `Width x Depth` | RAM | Error bound | Confidence |
  | --- | --- | --- | --- | --- | --- |
  | 20 | 2 | 55 x 5 | 550 B | 5 % of `N` | 99.3 % |
  | 50 | 2 | 136 x 5 | 1360 B | 2 % of `N` | 99.3 % |
  | 100 | 2 | 272 x 5 | 2720 B | 1 % of `N` | 99.3 % |
  | 100 | 3 | 272 x 7 | 3808 B | 1 % of `N` | 99.9 % |

  The bound is relative to the whole stream. Frequent keys get accurate estimates; rare keys can look much more frequent than they are.
- **Update rule.** `conservative` raises a counter only when the key needs it, so it overestimates less, often by a large factor on skewed streams. It costs a second pass over the rows. Use `standard` when sketches must merge exactly.
- **Saturation.** Counters saturate at `max_count()` and `total()` at `UINT64_MAX`. A saturated estimate means "at least `max_count()`". Pick `uint8_t` or `uint16_t` counters to save RAM when counts stay small, or call `halve()` periodically.
- **Hashing.** Each operation calls `Hash` once. The `Depth` row columns come from enhanced double hashing (`detail::double_hash_probe`), the same scheme as [`bloom_filter`](bloom_filter.md). A power-of-two `Width` turns the two initial reductions into masks. Two keys with equal `Hash` results share all counters.
- **Removal.** Counts cannot be subtracted: counters are shared between keys. Use `clear()` or `halve()`.
- **merge.** Both sketches must use the same `Width`, `Depth`, `Counter`, `Update`, and an equivalent hasher; the type enforces all but the hasher.
- **Persistence.** Save `counters().data()` (`counter_count()` counters in host order) and `total()`, then restore with `load_counters()`. Results stay stable across builds only when `size_type` and the hash function stay the same.
- `epsilon()`, `delta()`, and `error_bound()` use `float`. Every other operation is integer-only.
- Not thread-safe. Concurrent `estimate()` calls are safe only when no thread writes.
