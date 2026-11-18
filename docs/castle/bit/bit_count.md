# BitCount

## Overview
Bit-counting helpers for population counts, zero counts, bit widths, and parity. Use this header when code needs deterministic bit metrics without lookup tables or compiler-specific intrinsics.

## Header
`#include "castle/bit/bit_count.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [types](../core/types.md)
- [traits](../core/traits.md)

## Public API
| Signature | Description |
|---|---|
| `uint32_t popcount(uint64_t value)` | Counts set bits in a 64-bit value with a branch-free SWAR reduction. Complexity: O(1). |
| `template <typename T> uint32_t popcount(T value)` | Counts set bits in the corresponding unsigned representation of `value`. Complexity: O(1). |
| `template <typename T> uint32_t count_ones(T value)` | Alias for `popcount(value)`. Complexity: O(1). |
| `template <typename T> uint32_t count_zeros(T value)` | Counts zero bits across the full width of `T`. Complexity: O(1). |
| `template <typename T> uint32_t count_leading_zeros(T value)` | Counts consecutive zero bits from the most-significant side. Returns the full bit width when `value == 0`. Complexity: O(log bit-width). |
| `template <typename T> uint32_t count_trailing_zeros(T value)` | Counts consecutive zero bits from the least-significant side. Returns the full bit width when `value == 0`. Complexity: O(log bit-width). |
| `template <typename T> uint32_t bit_width(T value)` | Returns the number of bits needed to represent the corresponding unsigned value. Returns `0` for `0`. Complexity: O(log bit-width). |
| `template <typename T> uint32_t log2_floor(T value)` | Returns `bit_width(value) - 1` for non-zero values. This implementation returns `0` for `0` even though the mathematical operation is undefined there. Complexity: O(log bit-width). |
| `template <typename T> uint32_t parity(T value)` | Returns `1` for an odd number of set bits and `0` for an even number. Complexity: O(1). |
| `template <size_type N> uint32_t parity()` | Compile-time parity helper for `castle::size_type` constants. Complexity: O(1). |

## Usage Example
```cpp
#include "castle/bit/bit_count.hpp"

const uint8_t sample = 0x30U;
const uint32_t ones = castle::bit::count_ones(sample);
const uint32_t leading = castle::bit::count_leading_zeros(sample);
const uint32_t odd_parity = castle::bit::parity<7U>();
```
See `samples/sample_bit_count.cpp` for a complete example.

## Constraints & Notes
- Accepts Castle valid integer types only.
- Generic overloads interpret signed inputs through their corresponding unsigned representation.
- Zero handling is explicit: leading/trailing zero counts return the full width, `bit_width(0)` returns `0`, and `log2_floor(0)` returns `0` defensively.
