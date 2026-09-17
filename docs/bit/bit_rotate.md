# BitRotate

## Overview
Circular bit-rotation helpers for integer values. Use this header when bits that leave one side of a value must re-enter on the other side instead of being discarded.

## Header
`#include "castle/bit/bit_rotate.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)

## Public API
| Signature | Description |
|---|---|
| `template <typename T> T rotate_left(T value, uint32_t shift)` | Rotates `value` left by `shift` bit positions within the full width of `T`. The shift count is reduced modulo the bit width. Complexity: O(1). |
| `template <typename T> T rotate_right(T value, uint32_t shift)` | Rotates `value` right by `shift` bit positions within the full width of `T`. The shift count is reduced modulo the bit width. Complexity: O(1). |

## Usage Example
```cpp
#include "castle/bit/bit_rotate.hpp"

const uint32_t left = castle::bit::rotate_left(0x12345678U, 8U);
const uint32_t right = castle::bit::rotate_right(0x12345678U, 8U);
```
See `samples/sample_bit_rotate.cpp` for a complete example.

## Constraints & Notes
- Accepts Castle valid integer types only.
- A zero effective shift returns the original value so the implementation never shifts by the full bit width.
- Rotation is independent of machine endianness because it operates on the value, not on its memory layout.
