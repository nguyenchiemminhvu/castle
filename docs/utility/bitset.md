# Bitset

## Overview
`castle::bitset` is a fixed-size bit container with configurable storage word type. It exists for register images, protocol flags, and other embedded bit-manipulation tasks where capacity, storage, and behavior must remain deterministic.

## Header
`#include "castle/utility/bitset.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [types.md](../core/types.md), [traits.md](../core/traits.md)

## Public API
| API | Description |
|---|---|
| `bitset<Bits, Word>` | Fixed-size bit container using unsigned integral `Word` storage. |
| `bitset::reference` | Proxy returned by writable `operator[]` for single-bit assignment and flip operations. |
| `bitset()` / `bitset(unsigned long long)` / `bitset(const char*)` | Construct an empty bitset, load from an integer, or parse from text. |
| `size()`, `number_of_words()`, `bits_per_word()` | Compile-time capacity and storage queries. |
| `data()` | Accesses the underlying storage words. |
| `test()`, `any()`, `none()`, `all()`, `count()` | Inspect bit state. |
| `set()`, `reset()`, `flip()`, `assign()`, `from_string()` | Modify bits in bulk or by position. |
| `to_ulong()`, `to_ullong()`, `to_string()` | Export the current bits to an integer or caller-owned buffer. |
| `find_first()`, `find_next()`, `npos()` | Search for the next bit matching a requested state. |
| `operator[]`, `operator&=`, `operator|=`, `operator^=`, `operator<<=`, `operator>>=` | Element access and in-place bitwise/shift operations. |
| Non-member `operator&`, `operator|`, `operator^`, `operator<<`, `operator>>`, `operator~`, `operator==`, `operator!=`, `swap` | Value-returning operators and comparisons. |

## Usage Example
See `samples/sample_bitset.cpp`.

```cpp
castle::bitset<8U> flags(0x12U);
flags.set(0U).flip(4U);
```

## Constraints & Notes
- Fixed capacity known at compile time; no heap allocation.
- Out-of-range indexed writes are ignored and reads return `false`.
- `to_ulong()` and `to_ullong()` require the target integer type to be wide enough at compile time.
- `to_string()` writes into a caller-provided buffer and returns `false` when it is null or too small.
