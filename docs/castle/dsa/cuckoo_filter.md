# cuckoo_filter

## Overview
Fixed-size cuckoo filter: a probabilistic set that stores short fingerprints, not keys. Like [`bloom_filter`](bloom_filter.md) it answers "was this key inserted?" with:

- **no false negatives**: an inserted key that was not erased is always reported present;
- rare **false positives**: an absent key is reported present with probability about `2 * b * a / (2^f - 1)`.

Unlike a Bloom filter, it also supports `erase()` and `count()`, and it needs fewer bits per key at low false-positive rates. Inserts can fail when the table is nearly full; the failure is reported with `status::full` and leaves the filter unchanged. Typical embedded uses:

- Track active sessions, connections, or subscriptions that open and close at runtime.
- Drop duplicate frames inside a sliding window: insert on arrival, erase when the frame leaves the window.
- Cache which flash or EEPROM records exist while records are written and deleted.
- Block and unblock node IDs or addresses without a full table.

How it works (partial-key cuckoo hashing, Fan et al.):

1. One `Hash` call gives a bucket `i1` (low bits) and a fingerprint `f` (high bits of a 64-bit remix, never 0).
2. The second bucket is `i2 = i1 XOR hash(f)`. Applying the XOR again gives `i1` back, so either bucket finds the other from `f` alone.
3. `insert()` stores `f` in a free slot of `i1` or `i2`. When both are full, it swaps `f` with a stored fingerprint and moves that one to *its* alternate bucket, up to `MaxKicks` times.
4. If no free slot is found, every swap is undone in reverse order. No stored fingerprint is ever lost. Each step can be undone without a log, because the previous bucket is `alternate(bucket, carried)` and the victim slot is a pure function of bucket and step.

Theory: [Fan, Andersen, Kaminsky, Mitzenmacher: Cuckoo Filter: Practically Better Than Bloom](https://www.cs.cmu.edu/~dga/papers/cuckoo-conext2014.pdf), [Redis: Cuckoo filter](https://redis.io/docs/latest/develop/data-types/probabilistic/cuckoo-filter/).

## Header
`#include "castle/dsa/cuckoo_filter.hpp"`

## Dependencies
- [bit_math](../bit/bit_math.md) (`bit::next_power_of_two` for `cuckoo_bucket_count()`)
- [compiler](../core/compiler.md)
- [status](../core/status.md) (`castle::status` results)
- [traits](../core/traits.md) (`meta::is_power_of_two`, `meta::is_integral`, `meta::is_unsigned`, `meta::is_same`)
- [type_ranges](../core/type_ranges.md) (`castle::numeric_limits`)
- [types](../core/types.md)
- [grid](grid.md) (fingerprint table)
- [hash](../utility/hash.md) (`castle::hash`, `detail::hash_mix64`)
- [swap](../utility/swap.md) (`castle::swap` for relocations)

## Public API
### Sizing function
| API | Description |
| --- | --- |
| `constexpr size_type cuckoo_bucket_count(size_type expected_keys, size_type bucket_size)` | Returns `next_power_of_two(ceil(expected_keys / (0.9 * bucket_size)))`, at least 1: the bucket count that holds `expected_keys` at most 90 % full. Integer-only. |

### `cuckoo_filter<Key, Buckets, BucketSize = 4, Fingerprint = uint16_t, MaxKicks = 128, Hash = castle::hash<Key>>`
| API | Description |
| --- | --- |
| `cuckoo_filter<...>` | `Buckets` buckets of `BucketSize` fingerprint slots. Requires a power-of-two `Buckets`, `BucketSize > 0`, `2 * BucketSize < 2^f - 1`, and `Fingerprint` in `uint8_t`, `uint16_t`, `uint32_t`. `Hash` returns `size_type` from a `const` `operator()`. |
| `key_type`, `fingerprint_type`, `hasher`, `size_type`, `table_type` | `Key`, `Fingerprint`, `Hash`, `castle::size_type`, `grid<Fingerprint, Buckets, BucketSize>`. |
| `cuckoo_filter()` | Empty filter with a default-constructed `Hash`. |
| `explicit cuckoo_filter(const Hash& hash)` | Empty filter with a caller-supplied hasher, for example a seeded one. |
| `static constexpr size_type bucket_count()`, `bucket_size()`, `capacity()`, `max_kicks()` | `Buckets`, `BucketSize`, `Buckets * BucketSize`, `MaxKicks`. |
| `size_type size() const` | Number of stored fingerprints. |
| `bool empty() const` | `size() == 0`. |
| `float load_factor() const` | `size() / capacity()`. |
| `status insert(const Key& key)` | Stores one more fingerprint of `key`. Returns `status::ok`, or `status::full` when no free slot is reached within `MaxKicks` relocations; the filter is then unchanged. Duplicates are allowed. |
| `status insert_unique(const Key& key)` | Like `insert()`, but returns `status::already_exists` without change when `contains(key)`. Keeps one fingerprint per key. |
| `bool contains(const Key& key) const` | `false`: certainly absent. `true`: inserted and not erased, or a false positive. O(BucketSize). |
| `size_type count(const Key& key) const` | Matching fingerprints in both candidate buckets: at least the number of live inserts of `key`. |
| `status erase(const Key& key)` | Removes one matching fingerprint. Returns `status::ok` or `status::not_found`. |
| `void clear()` | Removes every fingerprint. |
| `float false_positive_rate() const` | `2 * size() / (Buckets * (2^f - 1))`, the expected false-positive probability at the current load. |
| `status load_table(const Fingerprint* fingerprints, size_type count)` | Restores `capacity()` slots and recounts `size()`. Returns `status::invalid_argument` if `fingerprints == nullptr`, `status::out_of_range` if `count != capacity()`, or `status::ok`. The filter is unchanged on failure. |
| `const table_type& table() const` | Read-only fingerprint table, zero for a free slot. `table().data()` exposes it for persistence or transfer. |
| `const hasher& hash_function() const` | Stored hash functor. |
| `operator==`, `operator!=` | Compares the slots. Equal key sets inserted in a different order may compare unequal. Hash functors are not compared. |

### `cuckoo_filter_for<Key, ExpectedKeys, Fingerprint = uint16_t, Hash = castle::hash<Key>>`
| API | Description |
| --- | --- |
| `cuckoo_filter_for<...>` | Alias of `cuckoo_filter<Key, cuckoo_bucket_count(ExpectedKeys, 4), 4, Fingerprint, 128, Hash>`. Sizes the filter from the expected key count. |

## Usage Example
```cpp
#include "castle/dsa/cuckoo_filter.hpp"

// 100 sessions: 32 buckets x 4 slots of 16 bits (256 bytes), about 0.01 % false positives.
castle::dsa::cuckoo_filter_for<uint32_t, 100U> sessions;

if (sessions.insert_unique(source_address) == castle::status::full)
{
    // Table too full: reject the new session or grow the filter at build time.
}

if (sessions.contains(request_source))
{
    // Maybe an open session: forward the request.
}

sessions.erase(source_address); // session closed
```

`samples/castle/dsa/cuckoo_filter.cpp` tracks diagnostic sessions on a gateway with `insert_unique()`, `contains()`, and `erase()`, and drops retransmitted frames inside a sliding window of 16 sequence numbers.

## Constraints & Notes
- No allocation, exceptions, RTTI, or virtual dispatch. The object is one `grid<Fingerprint, Buckets, BucketSize>` (`Buckets * BucketSize * sizeof(Fingerprint)` bytes), one `size_type`, and one `Hash` object. The filter is copyable.
- **False positives.** A lookup compares the fingerprint with up to `2 * BucketSize` stored ones. With load factor `a`, the rate is about `2 * b * a / (2^f - 1)`. With `b = 4` at 95 % load (`uint8_t` and `uint16_t` measured, `uint32_t` from the formula):

  | `Fingerprint` | Bytes per slot | False positives |
  | --- | --- | --- |
  | `uint8_t` | 1 | 3.0 % |
  | `uint16_t` | 2 | 0.012 % |
  | `uint32_t` | 4 | 0.0000002 % |

- **Load and `status::full`.** With `MaxKicks = 128`, inserts succeed up to a measured load of about 86 % for `BucketSize = 2`, 95 % for 4, and 98 % for 8. `cuckoo_bucket_count()` leaves 10 % headroom for 4-slot buckets. `MaxKicks = 0` disables relocation.
- **Timing.** `contains()`, `count()`, and `erase()` read two buckets: O(BucketSize). `insert()` is O(BucketSize) while a candidate bucket has room. Its worst case is bounded and deterministic: `MaxKicks` swaps plus the same number of undo swaps.
- **Erase only inserted keys.** Erasing a key that was never inserted can remove the colliding fingerprint of another key and create a false negative. Use `insert_unique()` when each key must hold exactly one fingerprint.
- **Duplicates.** `insert()` stores a fingerprint per call. A key fits at most `2 * BucketSize` times; then `insert()` returns `status::full`.
- **Hashing.** Each operation calls `Hash` once. The fingerprint comes from an independent 64-bit remix (`detail::hash_mix64`), so a 32-bit `size_type` or a weak user hash still gives uncorrelated buckets and fingerprints. Two keys with equal `Hash` results are indistinguishable.
- **Persistence.** Save `table().data()` (`capacity()` fingerprints in host order) and restore with `load_table()`. Results stay stable across builds only when `size_type` and the hash function stay the same.
- No merge: two cuckoo filters cannot be combined in place, unlike Bloom filters.
- `load_factor()` and `false_positive_rate()` use `float`. Every other operation is integer-only.
- Not thread-safe. Concurrent `contains()` calls are safe only when no thread writes.
