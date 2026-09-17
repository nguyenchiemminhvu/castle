# BitCore

## Overview
Helpers for testing and changing individual bits in integer values. Use this header for register shadows, packed protocol fields, and similar low-level code that needs explicit single-bit operations.

## Header
`#include "castle/bit/bit_core.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [types](../core/types.md)
- [traits](../core/traits.md)

## Public API
| Signature | Description |
|---|---|
| `template <typename T> bool test(T value, uint32_t bit_index)` | Returns whether the zero-based bit at `bit_index` is set. Out-of-range runtime indexes return `false`. Complexity: O(1). |
| `template <size_type bit_index, typename T> bool test(T value)` | Compile-time-index variant of `test`. Rejects invalid indexes with `static_assert`. Complexity: O(1). |
| `template <typename T> T set(T value, uint32_t bit_index)` | Returns `value` with the selected bit set. Out-of-range runtime indexes leave the value unchanged. Complexity: O(1). |
| `template <size_type bit_index, typename T> T set(T value)` | Compile-time-index variant of `set`. Complexity: O(1). |
| `template <typename T> T clear(T value, uint32_t bit_index)` | Returns `value` with the selected bit cleared. Out-of-range runtime indexes leave the value unchanged. Complexity: O(1). |
| `template <size_type bit_index, typename T> T clear(T value)` | Compile-time-index variant of `clear`. Complexity: O(1). |
| `template <typename T> T toggle(T value, uint32_t bit_index)` | Returns `value` with the selected bit inverted. Out-of-range runtime indexes leave the value unchanged. Complexity: O(1). |
| `template <size_type bit_index, typename T> T toggle(T value)` | Compile-time-index variant of `toggle`. Complexity: O(1). |

## Usage Example
```cpp
#include "castle/bit/bit_core.hpp"

uint32_t control = 0U;
control = castle::bit::set(control, 5U);
const bool enabled = castle::bit::test<5U>(control);
control = castle::bit::clear(control, 5U);
```
See `samples/sample_bit_core.cpp` for a complete example.

## Constraints & Notes
- Accepts Castle valid integer types only.
- Bit indexes are zero-based and operate on the full width of `T`.
- Generic operations cast through the corresponding unsigned type before shifting, avoiding signed right-shift semantics.
