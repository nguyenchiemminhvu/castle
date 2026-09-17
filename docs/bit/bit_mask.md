# `castle/bit/bit_mask.h` — Mask factories

**Header:** `castle/bit/bit_mask.h`
**Namespace:** `castle::bit`
**Sample:** [`samples/sample_bit.cpp`](../../samples/sample_bit.cpp) — `demo_bit_mask()`

## Purpose

Compile-time and runtime **factories** for the four masks needed most
often when writing bare-metal driver code:

- all-ones mask (`~T{0}`),
- single-bit mask,
- lowest-N-bits mask,
- highest-N-bits mask,
- arbitrary contiguous range mask.

## Design notes

- Every mask is available in **three flavours**:
  1. `foo<T>(runtime_index)` — plain `constexpr` function.
  2. `foo_const<index, T>` — trait struct exposing `value`.
  3. `foo_v<index, T>` — inline `constexpr` variable template shortcut.
- Out-of-range widths saturate to sensible values (either 0 or all-ones)
  rather than triggering UB via oversized shifts.
- Requires `castle::types::is_valid_integer_v<T>`.

## API

| Factory                                                | Description                                  |
| ------------------------------------------------------ | -------------------------------------------- |
| `all_bits_mask_v<T>`                                   | `~T{0}` at compile time                      |
| `single_bit_mask<T>(idx)` / `single_bit_mask_v<idx,T>` | `1 << idx`                                   |
| `low_bits_mask<T>(n)`  / `low_bits_mask_v<n,T>`        | `n` lowest bits set                          |
| `high_bits_mask<T>(n)` / `high_bits_mask_v<n,T>`       | `n` highest bits set                         |
| `range_mask<T>(start, width)` / `range_mask_v<s,w,T>`  | `width` contiguous bits starting at `start`  |

## Example — packing a hardware register

```cpp
#include <castle/bit/bit_mask.h>
using namespace castle::bit;

// TXFIFO_CTRL layout: [ threshold:6 | reserved:2 ]
constexpr uint8_t threshold_mask = low_bits_mask_v<6, uint8_t>;   // 0x3F
constexpr uint8_t reserved_mask  = high_bits_mask_v<2, uint8_t>;  // 0xC0

uint8_t regval = 0;
regval |= (threshold & threshold_mask);      // safe threshold write
regval &= static_cast<uint8_t>(~reserved_mask); // clear reserved bits
```

## See also

- `bit_utils.h` — `extract_field` / `insert_field` already combine
  these masks with the right shifts.
