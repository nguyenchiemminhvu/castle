# `castle/bit/bit_math.h` — Bit-based arithmetic helpers

**Header:** `castle/bit/bit_math.h`
**Namespace:** `castle::bit`
**Sample:** [`samples/sample_bit.cpp`](../../samples/sample_bit.cpp) — `demo_bit_math()`

## Purpose

Small numeric predicates and rounding routines that fall out naturally
from bit patterns and appear again and again in embedded code:
parity, powers of two, alignment, sign.

## Design notes

- Branch-free where possible (`is_even`, `is_odd`, `sign`).
- Every helper is `constexpr` and comes in both **runtime** and
  **compile-time** (`_v<N>`) forms — you can use them in `static_assert`,
  in enum sizing, and in `if constexpr` selectors.
- `align_up` / `align_down` / `is_aligned` **assume the alignment is a
  power of two**; enforcing that is the caller's responsibility (this is
  the common contract in linker scripts and DMA descriptor setup).

## API

| Function                            | Returns  | Notes                                    |
| ----------------------------------- | -------- | ---------------------------------------- |
| `is_even(v)` / `is_even_v<N>`       | `bool`   | LSB test                                 |
| `is_odd(v)` / `is_odd_v<N>`         | `bool`   | LSB test                                 |
| `is_power_of_two(v)` / `_v<N>`      | `bool`   | `false` for `0` and negative             |
| `next_power_of_two(v)` / `<N>()`    | `T`      | `1` for input `0`                        |
| `previous_power_of_two(v)` / `<N>()`| `T`      | `0` for input `0`                        |
| `align_up(v, align)`                | `T`      | `align` must be a power of two           |
| `align_down(v, align)`              | `T`      | `align` must be a power of two           |
| `is_aligned(v, align)`              | `bool`   | `align` must be a power of two           |
| `sign(v)` / `sign_v<N>`             | `int`    | `-1`, `0`, or `+1`, branch-free          |

All functions require `castle::types::is_valid_integer_v<T>`.

## Example — sizing a DMA descriptor ring

```cpp
#include <castle/bit/bit_math.h>
using namespace castle::bit;

// User asked for 200 descriptors; round up so hardware pointer math
// can be done with a single mask instead of a modulo.
constexpr std::size_t requested = 200;
constexpr std::size_t ring_size = next_power_of_two_v<requested>; // 256
static_assert(is_power_of_two_v<ring_size>);

std::uintptr_t base = reinterpret_cast<std::uintptr_t>(&ring[0]);
std::uintptr_t aligned = align_up(base, static_cast<std::uintptr_t>(64));
```

## See also

- `bit_count.h` — `log2_floor` / `bit_width` complement this module.
- `bit_utils.h` — extracting the lowest set bit is another idiomatic
  power-of-two operation.
