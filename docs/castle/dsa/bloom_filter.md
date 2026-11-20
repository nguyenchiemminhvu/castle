# bloom_filter

## Overview
Fixed-size Bloom filter: a probabilistic set that stores bits, not keys. It answers "is this key in the set?" with one of two answers:

- `false`: the key was **certainly never** inserted (no false negatives).
- `true`: the key was inserted, **or** it is a false positive.

A Bloom filter of `m` bits uses `k` hash positions per key. `insert()` sets the `k` bits of a key; `contains()` checks them and stops at the first clear bit. Keys cannot be listed or erased. Typical embedded uses:

- Keep a small filter in RAM in front of a slow flash, EEPROM, or remote lookup. Absent keys never touch slow storage.
- Drop unknown CAN identifiers, DTCs, or device IDs before a full table search.
- Exchange compact set summaries between nodes and combine them with `merge()`.

Theory and reference designs: [How to write a Bloom filter in C++](https://michaelschmatz.com/posts/how-to-write-a-bloom-filter-cpp/), [ArashPartow/bloom](https://github.com/ArashPartow/bloom), [fabiogaluppo/bloom_filter](https://github.com/fabiogaluppo/bloom_filter).

## Header
`#include "castle/dsa/bloom_filter.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [status](../core/status.md) (`castle::status` for `load_words()`)
- [types](../core/types.md)
- [initializer_list](initializer_list.md)
- [powi](../math/powi.md) (`math::powi`)
- [bitset](../utility/bitset.md) (bit storage)
- [hash](../utility/hash.md) (`castle::hash`, `detail::double_hash_probe`)

## Public API
### `bloom_optimal_hash_count(bits, expected_keys)`
| API | Description |
| --- | --- |
| `constexpr size_type bloom_optimal_hash_count(size_type bits, size_type expected_keys)` | Returns `round(bits / expected_keys * ln 2)`, at least 1. Returns 1 when `expected_keys == 0`. Integer-only, so it can feed a template argument. |

### `bloom_filter<Key, Bits, HashCount, Hash = castle::hash<Key>>`
| API | Description |
| --- | --- |
| `bloom_filter<Key, Bits, HashCount, Hash>` | `Bits` (`m`) bits and `HashCount` (`k`) probes per key. Requires `Bits > 0`, `1 <= HashCount <= Bits`, and `Bits <= size_type(-1) / 2`. `Hash` returns `size_type` from a `const` `operator()`. |
| `key_type`, `hasher`, `size_type`, `bitset_type`, `word_type` | `Key`, `Hash`, `castle::size_type`, `castle::bitset<Bits>`, `bitset_type::word_type`. |
| `bloom_filter()` | Empty filter with a default-constructed `Hash`. |
| `explicit bloom_filter(const Hash& hash)` | Empty filter with a caller-supplied hasher, for example a seeded one. |
| `bloom_filter(initializer_list<Key> list)` | Filter holding every key of `list`. |
| `static constexpr size_type bit_size()` | `Bits`. |
| `static constexpr size_type hash_count()` | `HashCount`. |
| `static constexpr size_type word_count()` | Number of storage words in `bits().data()`. |
| `bool insert(const Key& key)` | Sets the `k` bits of `key`. Returns `true` when at least one bit changed, so `key` was certainly new. Returns `false` when every bit was already set: a duplicate or a false positive. O(k). |
| `bool contains(const Key& key) const` | `false` when `key` was certainly never inserted; `true` otherwise. O(k), stops at the first clear bit. |
| `void merge(const bloom_filter& other)` | Bitwise OR: the result equals one filter that received both key sets. O(m / 32). |
| `void intersect(const bloom_filter& other)` | Bitwise AND: keys inserted into both filters still test `true`. The false-positive rate can exceed that of a filter built from the true intersection. O(m / 32). |
| `bool is_subset_of(const bloom_filter& other) const` | `true` when every set bit in `*this` is also set in `other`. O(m / 32), zero temporary storage. |
| `status load_words(const word_type* words, size_type count)` | Copies `word_count()` raw words into the filter and masks off any unused trailing bits in the last word. Returns `status::invalid_argument` if `words == nullptr`, `status::out_of_range` if `count != word_count()`, or `status::ok`. Filter is unchanged on failure. |
| `void clear()` | Clears every bit. |
| `bool empty() const` | `true` when every bit is clear. |
| `bool full() const` | `true` when every bit is set (filter is saturated). |
| `size_type bit_count() const` | Number of set bits, `0 .. Bits`. |
| `float fill_ratio() const` | Fraction of set bits, `bit_count() / Bits`, in `[0, 1]`. |
| `float false_positive_rate() const` | Current estimate `(bit_count() / Bits) ^ HashCount`. Uses the real fill, so duplicates and merges are taken into account. |
| `const bitset_type& bits() const` | Read-only storage. `bits().data()` exposes the words for persistence or transfer. |
| `const hasher& hash_function() const` | Returns the stored hash functor, for example to check seeds before `merge()`. |
| `operator==`, `operator!=` | Compares two filters by their bit states. Hash functors are not compared. |

### `bloom_filter_for<Key, ExpectedKeys, BitsPerKey = 10, Hash = castle::hash<Key>>`
| API | Description |
| --- | --- |
| `bloom_filter_for<Key, ExpectedKeys, BitsPerKey, Hash>` | Alias of `bloom_filter<Key, ExpectedKeys * BitsPerKey, bloom_optimal_hash_count(ExpectedKeys * BitsPerKey, ExpectedKeys), Hash>`. Sizes the filter from the expected key count. |

## Usage Example
```cpp
#include "castle/dsa/bloom_filter.hpp"

// About 64 keys at 10 bits per key: 640 bits (80 bytes), 7 probes, ~0.8 % false positives.
castle::dsa::bloom_filter_for<uint32_t, 64U> known_ids;
known_ids.insert(0x18DAF110U);

if (known_ids.contains(frame_id))
{
    // Maybe present: confirm in the slow table.
}
else
{
    // Certainly absent: skip the slow table.
}

// Explicit m and k, merged summaries from two nodes.
castle::dsa::bloom_filter<uint16_t, 256U, 4U> body{0x9A01U, 0x9A02U};
castle::dsa::bloom_filter<uint16_t, 256U, 4U> gateway;
gateway.merge(body);
```

`samples/castle/dsa/bloom_filter.cpp` puts a filter in front of a simulated flash key table and counts flash reads. It shows that every stored key is still found and that unknown keys almost never reach flash. It also counts distinct fault codes with the `insert()` result, merges two ECU filters, and stores `string_view` serial numbers.

## Constraints & Notes
- No allocation, exceptions, RTTI, or virtual dispatch. The object is one `castle::bitset<Bits>` (`ceil(Bits / 32)` 32-bit words) plus one `Hash` object. The filter is copyable.
- **False positives.** After `n` distinct keys, the false-positive probability is close to `p = (1 - e^(-k*n/m))^k`. For a fixed `m / n`, `k = (m / n) * ln 2` minimizes `p`. `bloom_filter_for` uses that `k`:

  | `BitsPerKey` (`m / n`) | `k` | `p` |
  | --- | --- | --- |
  | 4 | 3 | 14.7 % |
  | 6 | 4 | 5.6 % |
  | 8 | 6 | 2.2 % |
  | 10 | 7 | 0.82 % |
  | 12 | 8 | 0.31 % |
  | 16 | 11 | 0.046 % |
  | 20 | 14 | 0.007 % |

  Each extra ~4.8 bits per key cuts `p` by about 10 times. Inserting more than `ExpectedKeys` keys raises `p` gradually; nothing fails. Watch `false_positive_rate()` at run time.
- **Hashing.** Each operation calls `Hash` once. The `k` positions come from enhanced double hashing (Dillinger and Manolios): `x = h1 mod m`, `y = h2 mod m`, then `x += y; y += i` for every next probe. `h1` is the `Hash` result and `h2 = hash_mix64(h1)`. Both terms stay below `m`, so each step uses one compare-and-subtract, not a division. A power-of-two `Bits` turns the two initial reductions into masks.
- **Hash quality.** Two keys with equal `Hash` results share all `k` positions. `castle::hash` already mixes integral, enum, pointer, and `string_view` keys. A custom key needs a `castle::hash` specialization or a custom `Hash`.
- **Removal.** Clearing the bits of one key would create false negatives for other keys. Only `clear()` removes keys. A counting Bloom filter is outside this component.
- **merge / intersect.** Both filters must use the same `Bits`, `HashCount`, and an equivalent hasher; the type enforces the first two. Seeded hashers with different seeds give a meaningless result.
- **Persistence.** `bits().data()` holds `word_count()` words in host order; `load_words()` restores them and masks off any unused trailing bits in the last word. Results stay stable across builds only when `size_type` and the hash function stay the same.
- `fill_ratio()` and `false_positive_rate()` use `float`. Every other operation is integer-only.
- Not thread-safe. Concurrent `contains()` calls are safe only when no thread writes.
