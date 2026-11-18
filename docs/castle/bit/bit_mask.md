# BitMask

## Overview
Mask-building helpers for single bits and contiguous bit ranges. Use this header when defining register fields, protocol masks, or compact bitset layouts.

## Header
`#include "castle/bit/bit_mask.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)

## Public API
| Signature | Description |
|---|---|
| `template <typename T> struct all_bits_mask` | Exposes `all_bits_mask<T>::value`, a compile-time mask with every bit set. Complexity: O(1). |
| `template <typename T> T single_bit_mask(uint32_t bit_index)` | Returns a value with only the selected bit set. Caller must keep `bit_index` within range because this overload does not bounds-check before shifting. Complexity: O(1). |
| `template <size_type bit_index, typename T> struct single_bit_mask_const` | Compile-time single-bit mask with `single_bit_mask_const<bit_index, T>::value`. Invalid indexes fail with `static_assert`. Complexity: O(1). |
| `template <typename T> T low_bits_mask(uint32_t bit_count)` | Returns a mask whose lowest `bit_count` bits are set. Full-width-or-larger counts produce an all-ones mask. Complexity: O(1). |
| `template <size_type bit_index, typename T> struct low_bits_mask_const` | Compile-time low-bit mask with `low_bits_mask_const<bit_index, T>::value`. Complexity: O(1). |
| `template <typename T> T high_bits_mask(uint32_t bit_count)` | Returns a mask whose highest `bit_count` bits are set. A zero count returns `0`; a full-width-or-larger count returns all ones. Complexity: O(1). |
| `template <size_type bit_count, typename T> struct high_bits_mask_const` | Compile-time high-bit mask with `high_bits_mask_const<bit_count, T>::value`. Complexity: O(1). |
| `template <typename T> T range_mask(uint32_t start_bit_index, uint32_t bit_count)` | Returns a contiguous mask starting at `start_bit_index` and spanning `bit_count` bits. Caller must keep the shifted range valid for `T`. Complexity: O(1). |
| `template <size_type start_bit_index, size_type bit_count, typename T> struct range_mask_const` | Compile-time range mask with `range_mask_const<start_bit_index, bit_count, T>::value`. Invalid ranges fail with `static_assert`. Complexity: O(1). |

## Usage Example
```cpp
#include "castle/bit/bit_mask.hpp"

const uint8_t low = castle::bit::low_bits_mask<uint8_t>(5U);
const uint8_t high = castle::bit::high_bits_mask<uint8_t>(3U);
const uint8_t field = castle::bit::range_mask<uint8_t>(2U, 3U);
```
See `samples/sample_bit_mask.cpp` for a complete example.

## Constraints & Notes
- Accepts Castle valid integer types where the API is SFINAE-gated.
- Runtime `low_bits_mask` and `high_bits_mask` special-case full-width counts; some other builders shift directly and therefore rely on the caller to keep indexes in range.
- All operations are constant time and allocation free.
