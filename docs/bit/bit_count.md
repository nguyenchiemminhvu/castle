# `castle/bit/bit_count.h` — Counting & log2 helpers

**Header:** `castle/bit/bit_count.h`
**Namespace:** `castle::bit`
**Sample:** [`samples/sample_bit.cpp`](../../samples/sample_bit.cpp) — `demo_bit_count()`

## Purpose

Population counting and bit-scan primitives used across CRC tables,
parity checks, alignment math, and CAN/UDS diagnostic services.
Implementations are **branch-free, `constexpr`, and portable** — no
compiler intrinsics required.

## Design notes

- `popcount` uses a SWAR (SIMD Within A Register) algorithm on a
  64-bit lane; smaller integral types are widened to `uint64_t` first.
- `count_leading_zeros` / `count_trailing_zeros` use a `log2 N`
  binary-search reduction — deterministic time regardless of input.
- Every helper returns `sizeof(T) * CHAR_BIT` when the input is zero
  where "count zeros from that end" would otherwise be undefined.
- `log2_floor(0)` is defensively defined as `0` (mathematically
  undefined) so it never triggers UB in safety-critical code.

## API

| Function                                      | Returns          | Notes                                            |
| --------------------------------------------- | ---------------- | ------------------------------------------------ |
| `popcount(v)`                                 | `uint32_t`       | Number of 1-bits                                 |
| `count_ones(v)`                               | `uint32_t`       | Alias of `popcount`                              |
| `count_zeros(v)`                              | `uint32_t`       | `sizeof(T)*8 - popcount(v)`                      |
| `count_leading_zeros(v)`                      | `uint32_t`       | `sizeof(T)*8` for `v == 0`                       |
| `count_trailing_zeros(v)`                     | `uint32_t`       | `sizeof(T)*8` for `v == 0`                       |
| `bit_width(v)`                                | `uint32_t`       | `floor(log2(v)) + 1`, `0` for `v == 0`           |
| `log2_floor(v)`                               | `uint32_t`       | `floor(log2(v))`, `0` for `v == 0` (defensive)   |
| `parity(v)` / `parity<N>()`                   | `uint32_t`       | 1 if odd number of set bits, 0 otherwise         |

All functions require `castle::types::is_valid_integer_v<T>`.

## Example

```cpp
#include <castle/bit/bit_count.h>
using namespace castle::bit;

static_assert(popcount(0xFFu) == 8);
static_assert(count_leading_zeros<uint32_t>(0x1u) == 31);
static_assert(bit_width(0x0F) == 4);
static_assert(parity<0b1011>() == 1);   // odd
```

## See also

- `bit_math.h` — `is_power_of_two`, `next_power_of_two`.
- `bit_utils.h` — `extract_lowest_set_bit` / `extract_highest_set_bit`.
