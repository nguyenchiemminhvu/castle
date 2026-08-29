# `castle/bit/bit_core.h` — Single-bit read / write

**Header:** `castle/bit/bit_core.h`
**Namespace:** `castle::bit`
**Sample:** [`samples/sample_bit.cpp`](../../samples/sample_bit.cpp) — `demo_bit_core()`

## Purpose

Fundamental **per-bit** primitives: query, set, clear, and toggle a
single bit inside an integral value. These four operations are the
building blocks used everywhere else in `castle::bit`.

## Design notes

- `constexpr` + `noexcept`, branch-free on the fast path.
- Two overload flavours per operation:
  - **Runtime index:** `f(value, bit_index)`. Out-of-range indices
    return the input unchanged (`set`, `clear`, `toggle`) or `false`
    (`test`) — no UB, no exceptions.
  - **Compile-time index:** `f<bit_index>(value)`. A `static_assert`
    enforces `bit_index < sizeof(T) * CHAR_BIT`.
- Internally values are cast to their unsigned counterpart to make
  shift semantics well-defined for signed inputs.

## API

| Function                                    | Returns | Notes                        |
| ------------------------------------------- | ------- | ---------------------------- |
| `test(T v, uint32_t bit_index)`             | `bool`  | `false` if index OOR         |
| `test<bit_index>(T v)`                      | `bool`  | Compile-time checked         |
| `set(T v, uint32_t bit_index)`              | `T`     | Returns `v` if index OOR     |
| `set<bit_index>(T v)`                       | `T`     | Compile-time checked         |
| `clear(T v, uint32_t bit_index)`            | `T`     | Returns `v` if index OOR     |
| `clear<bit_index>(T v)`                     | `T`     | Compile-time checked         |
| `toggle(T v, uint32_t bit_index)`           | `T`     | Returns `v` if index OOR     |
| `toggle<bit_index>(T v)`                    | `T`     | Compile-time checked         |

All functions require `castle::types::is_valid_integer_v<T>`.

## Example

```cpp
#include <castle/bit/bit_core.h>
using namespace castle::bit;

uint32_t reg = 0b0000'1111;

bool b3   = test<3>(reg);       // true
reg       = set<5>(reg);        // 0b0010'1111
reg       = clear(reg, 0U);     // 0b0010'1110
reg       = toggle<4>(reg);     // 0b0011'1110
```

## See also

- `bit_mask.h` — build masks for multi-bit ranges.
- `bit_utils.h` — read / write **contiguous** bit fields at once.
