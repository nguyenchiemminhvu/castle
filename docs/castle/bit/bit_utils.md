# BitUtils

## Overview
Helpers for isolating set bits and extracting or inserting contiguous bit fields. Use this header for register decoding, packed protocol words, and other low-level field manipulations.

## Header
`#include "castle/bit/bit_utils.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)

## Public API
| Signature | Description |
|---|---|
| `template <typename T> T extract_lowest_set_bit(T value)` | Returns a value that keeps only the lowest set bit. Zero stays zero. Complexity: O(1). |
| `template <typename T> T extract_highest_set_bit(T value)` | Returns a value that keeps only the highest set bit. Zero stays zero. Complexity: O(log bit-width). |
| `template <typename T> T extract_field(T value, uint32_t start_bit, uint32_t width)` | Extracts a runtime-selected field, right-aligns it, and returns `0` for zero widths or out-of-range starting positions. Complexity: O(1). |
| `template <size_type start_bit, size_type width, typename T> T extract_field(T value)` | Compile-time field extractor. Invalid field sizes fail with `static_assert`. Complexity: O(1). |
| `template <typename T> T insert_field(T dest, T field_val, uint32_t start_bit, uint32_t width)` | Replaces a runtime-selected field inside `dest`, masking `field_val` down to `width` bits. Zero widths or out-of-range starting positions leave `dest` unchanged. Complexity: O(1). |
| `template <size_type start_bit, size_type width, typename T> T insert_field(T dest, T field_val)` | Compile-time field inserter. Invalid field sizes fail with `static_assert`. Complexity: O(1). |

## Usage Example
```cpp
#include "castle/bit/bit_utils.hpp"

const uint16_t field = castle::bit::extract_field(static_cast<uint16_t>(0xA5B6U), 4U, 4U);
const uint16_t updated = castle::bit::insert_field(static_cast<uint16_t>(0xFF00U), static_cast<uint16_t>(0x0005U), 4U, 4U);
```
See `samples/sample_bit_utils.cpp` for a complete example.

## Constraints & Notes
- Accepts Castle valid integer types only.
- Runtime field helpers guard zero widths and out-of-range starting positions, but callers should still choose ranges that make sense for the target type.
- Bit positions are numbered from the least-significant bit.
