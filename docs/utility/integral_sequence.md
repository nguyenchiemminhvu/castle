# Integral Sequence

## Overview
This header provides Castle's compile-time index-sequence utilities. It exists so tuple-like code and metaprogramming helpers can generate numeric index packs without `<utility>`.

## Header
`#include "castle/utility/integral_sequence.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [types.md](../core/types.md), [traits.md](../core/traits.md)

## Public API
| API | Description |
|---|---|
| `sequence::index_sequence<Indices...>` / `index_sequence<Indices...>` | Stores a pack of `size_type` indices. |
| `sequence::merge_and_renumber<Left, Right>` | Concatenates two index sequences and renumbers the second pack. |
| `sequence::make_index_sequence<N>` / `make_index_sequence<N>` | Builds `index_sequence<0, 1, ..., N - 1>`. |
| `sequence::make_index_sequence_t<N>` / `make_index_sequence_t<N>` | Alias for the generated sequence type. |
| `sequence::index_sequence_for<Ts...>` | Builds one index per type in `Ts...`. |
| `sequence::sequence_size<Sequence>` / `sequence_size_t<Sequence>` | Compile-time size of an index sequence. |
| `sequence::is_index_sequence<Sequence>` / `sequence::is_index_sequence_v<Sequence>` / `is_index_sequence<Sequence>` | Detects Castle index-sequence types. |

## Usage Example
See `samples/sample_integral_sequence.cpp`.

```cpp
using seq = castle::make_index_sequence_t<4U>;
static_assert(seq::size() == 4U, "sequence size");
```

## Constraints & Notes
- Purely compile-time utilities; no runtime storage or allocation.
- Sequence elements are always `castle::size_type`.
- The generated sequence order is contiguous and zero-based.
