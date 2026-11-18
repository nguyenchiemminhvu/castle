# BitReverse

## Overview
Helpers for reversing bit order or byte order in integer values. Use this header for bit-serial protocols, endianness conversion, and packed data transformations.

## Header
`#include "castle/bit/bit_reverse.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)

## Public API
| Signature | Description |
|---|---|
| `uint8_t reverse_bits(uint8_t n)` | Reverses all 8 bit positions. Complexity: O(1). |
| `uint16_t reverse_bits(uint16_t n)` | Reverses all 16 bit positions. Complexity: O(1). |
| `uint32_t reverse_bits(uint32_t n)` | Reverses all 32 bit positions. Complexity: O(1). |
| `uint64_t reverse_bits(uint64_t n)` | Reverses all 64 bit positions. Complexity: O(1). |
| `template <typename T> T reverse_bits(T value)` | Generic integer overload that reverses the bit positions of the corresponding unsigned representation and casts back to `T`. Complexity: O(1). |
| `uint8_t byte_swap(uint8_t v)` | Returns the byte unchanged. Complexity: O(1). |
| `uint16_t byte_swap(uint16_t v)` | Reverses the two-byte order. Complexity: O(1). |
| `uint32_t byte_swap(uint32_t v)` | Reverses the four-byte order. Complexity: O(1). |
| `uint64_t byte_swap(uint64_t v)` | Reverses the eight-byte order. Complexity: O(1). |
| `template <typename T> T byte_swap(T value)` | Generic integer overload that byte-swaps the corresponding unsigned representation and casts back to `T`. Complexity: O(1). |
| `template <typename T> T reverse_bytes(T value)` | Alias for `byte_swap(value)`. Complexity: O(1). |

## Usage Example
```cpp
#include "castle/bit/bit_reverse.hpp"

const uint8_t bits = castle::bit::reverse_bits(static_cast<uint8_t>(0xB0U));
const uint32_t bytes = castle::bit::byte_swap(0x12345678U);
```
See `samples/sample_bit_reverse.cpp` for a complete example.

## Constraints & Notes
- Accepts Castle valid integer types for the generic overloads.
- Bit reversal changes logical bit positions across the full width of the value; it is not an endianness operation.
- Byte swapping reverses byte order only; it does not reverse bits inside each byte.
