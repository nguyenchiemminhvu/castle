# hyper_log_log

## Overview
Fixed-size HyperLogLog sketch: estimates **how many distinct keys** a stream contained, in constant memory. Counting distinct keys exactly needs memory for every key seen. HyperLogLog needs `2^p` bytes, however long the stream and however many distinct keys it has. In exchange, the answer is an estimate with a relative standard error of about `1.04 / sqrt(2^p)`.

- Duplicates never change the estimate: adding a key a second time is free.
- Two sketches merge losslessly: the merged sketch equals one sketch fed with both streams (set union).
- Keys cannot be listed, looked up, or removed.

Typical embedded uses:

- Vehicle stands still: count how many distinct ~1 m position cells the GNSS fix wanders through.
- Count distinct CAN IDs, MAC addresses, or clients on a bus or a port per time window.
- Count distinct error codes, users, or devices per drive cycle, and combine counts from several ECUs.

How it works:

1. Each key hash is widened to 64 bits. Its top `p` bits pick one of `2^p` registers.
2. The rank of the remaining `64 - p` bits is one plus their number of leading zeros. A rank of `r` has probability `2^-r`, so long zero runs need many distinct keys.
3. Each register keeps the largest rank it saw. Duplicates produce the same rank, so they change nothing.
4. `estimate()` turns the register values into a cardinality with Ertl's improved estimator (also used by Redis). It counts empty registers `C0` and saturated registers `Cmax`, and sums `2^-r` over the others:

   `E = alpha * m^2 / (m * sigma(C0 / m) + sum + m * tau(1 - Cmax / m) / 2^(64 - p))`

   `sigma` and `tau` are short convergent series. One formula covers the whole range: no bias tables, no switch to linear counting for small counts.

Theory: [Flajolet, Fusy, Gandouet, Meunier: HyperLogLog](https://algo.inria.fr/flajolet/Publications/FlFuGaMe07.pdf), [Ertl: New cardinality estimation algorithms for HyperLogLog sketches](https://arxiv.org/abs/1702.01284), [Redis: HyperLogLog](https://redis.io/docs/latest/develop/data-types/probabilistic/hyperloglogs/).

## Header
`#include "castle/dsa/hyper_log_log.hpp"`

## Dependencies
- [bit_count](../bit/bit_count.md) (`bit::count_leading_zeros` for the rank)
- [compiler](../core/compiler.md)
- [status](../core/status.md) (`castle::status` for `load_registers()`)
- [type_ranges](../core/type_ranges.md) (`castle::numeric_limits`)
- [types](../core/types.md)
- [array](array.md) (register storage)
- [sqrt_real](../math/sqrt_real.md) (`math::sqrt_real` in `tau`)
- [hash](../utility/hash.md) (`castle::hash`, `detail::hash_mix64`)

## Public API
### `hyper_log_log<Key, Precision = 10, Hash = castle::hash<Key>>`
| API | Description |
| --- | --- |
| `hyper_log_log<Key, Precision, Hash>` | `2^Precision` one-byte registers. Requires `4 <= Precision <= 16`. `Hash` returns `size_type` from a `const` `operator()`. |
| `key_type`, `hasher`, `size_type`, `register_type`, `register_array` | `Key`, `Hash`, `castle::size_type`, `uint8_t`, `dsa::array<uint8_t, 2^Precision>`. |
| `hyper_log_log()` | Empty sketch with a default-constructed `Hash`. |
| `explicit hyper_log_log(const Hash& hash)` | Empty sketch with a caller-supplied hasher, for example a seeded one. |
| `static constexpr size_type precision()` | `Precision`. |
| `static constexpr size_type register_count()` | `2^Precision`, also the register size in bytes. |
| `static constexpr uint8_t max_rank()` | Largest register value, `64 - Precision + 1`. |
| `static constexpr float standard_error()` | `1.04 / sqrt(2^Precision)`. |
| `bool add(const Key& key)` | Adds a key. Returns `true` when a register grew, so the estimate may have changed; `false` for a key whose rank was already covered (for example a duplicate). O(1). |
| `uint64_t estimate() const` | Rounded number of distinct keys. Zero when empty. Saturates at `UINT64_MAX`. O(2^p), uses `double`. |
| `void merge(const hyper_log_log& other)` | Keeps the larger value of each register pair: the sketch of the union of both streams. O(2^p). |
| `status load_registers(const uint8_t* registers, size_type count)` | Restores `register_count()` registers. Returns `status::invalid_argument` if `registers == nullptr` or a value exceeds `max_rank()`, `status::out_of_range` if `count != register_count()`, or `status::ok`. The sketch is unchanged on failure. |
| `void clear()` | Zeros every register. |
| `bool empty() const` | `true` when every register is zero. O(2^p). |
| `const register_array& registers() const` | Read-only registers. `registers().data()` exposes them for persistence or transfer. |
| `const hasher& hash_function() const` | Stored hash functor, for example to check seeds before `merge()`. |
| `operator==`, `operator!=` | Compares the registers. Hash functors are not compared. |

## Usage Example
```cpp
#include "castle/dsa/hyper_log_log.hpp"

// 1024 registers (1 KiB), about 3.25 % standard error.
castle::dsa::hyper_log_log<uint64_t> positions;

// Quantize each fix to a ~1 m grid cell and pack it into one key.
const uint64_t cell = (static_cast<uint64_t>(lat_e7 / 100) << 32U) | static_cast<uint32_t>(lon_e7 / 100);
positions.add(cell); // repeated cells cost nothing

const uint64_t distinct_cells = positions.estimate();

// Gateway: union of the identifiers seen on two buses.
castle::dsa::hyper_log_log<uint32_t> bus_a;
castle::dsa::hyper_log_log<uint32_t> bus_b;
bus_a.merge(bus_b);
```

`samples/castle/dsa/hyper_log_log.cpp` counts the distinct ~1 m cells of a parked vehicle's GNSS fixes over one hour, and the distinct CAN identifiers of two buses before and after `merge()`.

## Constraints & Notes
- No allocation, exceptions, RTTI, or virtual dispatch. The object is one `dsa::array<uint8_t, 2^p>` plus one `Hash` object. The sketch is copyable.
- **Accuracy and memory.** The relative standard error is `1.04 / sqrt(2^p)`; about 99.7 % of estimates fall within three standard errors. Small counts are nearly exact: with `p = 10`, counts up to 64 are off by at most 2 in the unit test.

  | `Precision` | Registers (bytes) | Standard error |
  | --- | --- | --- |
  | 4 | 16 | 26 % |
  | 6 | 64 | 13 % |
  | 8 | 256 | 6.5 % |
  | 10 | 1024 | 3.25 % |
  | 12 | 4096 | 1.63 % |
  | 14 | 16384 | 0.81 % |
  | 16 | 65536 | 0.41 % |

- **Range.** Every hash is remixed to 64 bits (`detail::hash_mix64`), so ranks use `64 - p` bits on every target. With a 32-bit `size_type`, at most `2^32` distinct hash values exist; estimates stay accurate up to about `10^8` distinct keys.
- **Timing.** `add()` is O(1): one hash, one remix, one leading-zero count, one compare. `estimate()`, `merge()`, `empty()`, `clear()`, and `load_registers()` are O(2^p). `estimate()` runs two short series in `double` whose length depends only on the register counts; call it when the result is needed, not per key.
- **Hashing.** Each `add()` calls `Hash` once. Two keys with equal `Hash` results count as one key. A custom key needs a `castle::hash` specialization or a custom `Hash`.
- **Removal.** Registers are shared between keys, so a key cannot be removed. Use `clear()` per time window.
- **merge.** Both sketches must use the same `Precision` and an equivalent hasher; the type enforces the first. Merging is commutative and idempotent.
- **Persistence.** Save `registers().data()` (`register_count()` bytes) and restore with `load_registers()`, which rejects out-of-range values. Results stay stable across builds only when `size_type` and the hash function stay the same.
- Not thread-safe. Concurrent `estimate()` calls are safe only when no thread writes.
