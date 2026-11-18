# Flags

## Overview
Small masked flag-set wrapper for unsigned integral values. Use it when code needs a type-safe register shadow or feature bitset that always keeps stored bits within a compile-time mask.

## Header
`#include "castle/bit/flags.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)
- [move](../utility/move.md)

## Public API
- `template <typename T, T MASK = static_cast<T>(~T(0))> class flags` — Value-based masked flag wrapper for unsigned Castle integer types.
- Type and constants:
  - `using value_type = T` — underlying storage type.
  - `static constexpr value_type ALL_SET` — all mask-selected bits set.
  - `static constexpr value_type ALL_CLEAR` — all bits clear.
  - `static constexpr size_t NBITS` — storage width of `value_type` in bits.
- Constructors:
  - `flags()` — constructs an empty flag set. Complexity: O(1).
  - `flags(value_type pattern)` — stores `pattern & MASK`. Complexity: O(1).
  - `flags(const flags& other)` — copy constructor. Complexity: O(1).
  - `flags(flags&& other)` — move constructor. Complexity: O(1).
- Test/query methods:
  - `template <value_type pattern> bool test() const` — returns whether any bit from `pattern` is set. Complexity: O(1).
  - `bool test(value_type pattern) const` — runtime form of `test`. Complexity: O(1).
  - `bool all() const` — returns whether all mask-selected bits are set. Complexity: O(1).
  - `template <value_type pattern> bool all_of() const` — returns whether every mask-selected bit from `pattern` is set. Complexity: O(1).
  - `bool all_of(value_type pattern) const` — runtime form of `all_of`. Complexity: O(1).
  - `bool none() const` — returns whether no bits are set. Complexity: O(1).
  - `template <value_type pattern> bool none_of() const` — returns whether every mask-selected bit from `pattern` is clear. Complexity: O(1).
  - `bool none_of(value_type pattern) const` — runtime form of `none_of`. Complexity: O(1).
  - `bool any() const` — returns whether any bit is set. Complexity: O(1).
  - `template <value_type pattern> bool any_of() const` — returns whether any mask-selected bit from `pattern` is set. Complexity: O(1).
  - `bool any_of(value_type pattern) const` — runtime form of `any_of`. Complexity: O(1).
  - `value_type value() const` — returns the stored masked pattern. Complexity: O(1).
  - `operator value_type() const` — implicit conversion to the stored masked pattern. Complexity: O(1).
- Modifiers:
  - `template <value_type pattern, bool value> flags& set()` — compile-time pattern + compile-time boolean set/clear. Complexity: O(1).
  - `template <value_type pattern> flags& set(bool value)` — compile-time pattern + runtime boolean set/clear. Complexity: O(1).
  - `template <value_type pattern> flags& set()` — sets a compile-time pattern after masking it. Complexity: O(1).
  - `flags& set(value_type pattern)` — sets a runtime pattern after masking it. Complexity: O(1).
  - `flags& set(value_type pattern, bool value)` — runtime pattern + runtime boolean set/clear. Complexity: O(1).
  - `flags& clear()` — clears every bit. Complexity: O(1).
  - `template <value_type pattern> flags& reset()` — clears a compile-time pattern. Complexity: O(1).
  - `flags& reset(value_type pattern)` — clears a runtime pattern. Complexity: O(1).
  - `flags& flip()` — inverts all mask-selected bits. Complexity: O(1).
  - `template <value_type pattern> flags& flip()` — inverts a compile-time pattern after masking it. Complexity: O(1).
  - `flags& flip(value_type pattern)` — inverts a runtime pattern after masking it. Complexity: O(1).
  - `flags& value(value_type pattern)` — replaces the stored pattern with `pattern & MASK`. Complexity: O(1).
  - `void swap(flags& other)` — swaps two wrappers. Complexity: O(1).
- Operators:
  - `flags& operator&=(value_type pattern)` — bitwise-AND assignment. Complexity: O(1).
  - `flags& operator|=(value_type pattern)` — bitwise-OR assignment with `pattern & MASK`. Complexity: O(1).
  - `flags& operator^=(value_type pattern)` — bitwise-XOR assignment with `pattern & MASK`. Complexity: O(1).
  - `flags& operator=(const flags& other)` — copy assignment. Complexity: O(1).
  - `flags& operator=(value_type pattern)` — assigns `pattern & MASK`. Complexity: O(1).
  - `template <typename T, T MASK> bool operator==(const flags<T, MASK>& lhs, const flags<T, MASK>& rhs)` — compares stored masked values. Complexity: O(1).
  - `template <typename T, T MASK> bool operator!=(const flags<T, MASK>& lhs, const flags<T, MASK>& rhs)` — inequality comparison. Complexity: O(1).
  - `template <typename T, T MASK> void swap(flags<T, MASK>& lhs, flags<T, MASK>& rhs)` — free swap forwarding to the member swap. Complexity: O(1).

## Usage Example
```cpp
#include "castle/bit/flags.hpp"

using mode_flags = castle::bit::flags<uint8_t, 0x0FU>;

mode_flags flags(0x13U);
flags.set(0x04U).flip(0x01U);
const bool has_mode = flags.any_of(0x06U);
```
See `samples/sample_flags.cpp` for a complete example.

## Constraints & Notes
- `T` must be an unsigned Castle valid integer type.
- Operations that can introduce bits apply `MASK`, preserving the masked invariant.
- `NBITS` reports the storage width of `value_type`, not the number of bits enabled by `MASK`.
