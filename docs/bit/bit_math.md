# BitMath

## Overview
Integer math helpers expressed as bit operations. Use this header for parity tests, power-of-two rounding, alignment calculations, and sign queries that need to stay deterministic and constexpr-friendly.

## Header
`#include "castle/bit/bit_math.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [types](../core/types.md)
- [traits](../core/traits.md)

## Public API
| Signature | Description |
|---|---|
| `template <typename T> bool is_even(T v)` | Returns whether the least-significant bit is clear. Complexity: O(1). |
| `template <size_type N> struct is_even_const` | Compile-time evenness test via `is_even_const<N>::value`. Complexity: O(1). |
| `template <typename T> bool is_odd(T v)` | Returns whether the least-significant bit is set. Complexity: O(1). |
| `template <size_type N> struct is_odd_const` | Compile-time oddness test via `is_odd_const<N>::value`. Complexity: O(1). |
| `template <typename T> bool is_power_of_two(T v)` | Returns whether `v` is a positive exact power of two. Signed negative inputs return `false`. Complexity: O(1). |
| `template <size_type N> struct is_power_of_two_const` | Compile-time power-of-two test via `is_power_of_two_const<N>::value`. Complexity: O(1). |
| `template <typename T> T next_power_of_two(T v)` | Returns the smallest power of two greater than or equal to `v`; `0` maps to `1`. Complexity: O(log bit-width). |
| `template <size_type N> size_type next_power_of_two()` | Compile-time `castle::size_type` form of `next_power_of_two`. Complexity: O(log bit-width). |
| `template <size_type N> struct next_power_of_two_const` | Compile-time wrapper exposing `next_power_of_two_const<N>::value`. Complexity: O(log bit-width). |
| `template <typename T> T previous_power_of_two(T v)` | Returns the largest power of two less than or equal to `v`; `0` maps to `0`. Complexity: O(log bit-width). |
| `template <size_type N> size_type previous_power_of_two()` | Compile-time `castle::size_type` form of `previous_power_of_two`. Complexity: O(log bit-width). |
| `template <size_type N> struct previous_power_of_two_const` | Compile-time wrapper exposing `previous_power_of_two_const<N>::value`. Complexity: O(log bit-width). |
| `template <typename T> T align_up(T value, T alignment)` | Rounds `value` up to the next multiple of `alignment`. Caller is expected to pass a non-zero power-of-two alignment. Complexity: O(1). |
| `template <typename T> T align_down(T value, T alignment)` | Rounds `value` down to the previous multiple of `alignment`. Caller is expected to pass a non-zero power-of-two alignment. Complexity: O(1). |
| `template <typename T> bool is_aligned(T value, T alignment)` | Returns whether `value` is already aligned to `alignment`. Caller is expected to pass a non-zero power-of-two alignment. Complexity: O(1). |
| `template <typename T> int sign(T v)` | Returns `-1`, `0`, or `1` for negative, zero, or positive runtime values. Complexity: O(1). |
| `template <size_type N> int sign()` | Compile-time sign helper for `castle::size_type` values; returns `0` for `0` and `1` otherwise. Complexity: O(1). |

## Usage Example
```cpp
#include "castle/bit/bit_math.hpp"

const uint32_t capacity = castle::bit::next_power_of_two(300U);
const uint32_t base = castle::bit::align_down(37U, 8U);
const bool ready = castle::bit::is_power_of_two(capacity);
```
See `samples/sample_bit_math.cpp` for a complete example.

## Constraints & Notes
- Accepts Castle valid integer types only.
- `next_power_of_two` and `previous_power_of_two` are intended for non-negative values; overflow follows the implementation's unsigned arithmetic path.
- Alignment helpers do not validate the alignment argument.
