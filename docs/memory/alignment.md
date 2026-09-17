# Alignment

## Overview
Provides alignment checks and raw aligned storage building blocks for objects that will be constructed later in caller-owned memory. It exists so embedded code can reserve deterministic storage without heap allocation or STL facilities.

## Header
`#include "castle/memory/alignment.hpp"`

## Dependencies
- [Compiler](../core/compiler.md)
- [Traits](../core/traits.md)
- [Types](../core/types.md)

## Public API
| API | Description |
| --- | --- |
| `bool castle::memory::is_aligned(void const* p, size_type required_alignment)` | Runtime alignment check. Returns `false` for zero or non-power-of-two alignments. O(1). |
| `template <size_type Alignment> bool castle::memory::is_aligned(void const* p)` | Compile-time alignment check using `Alignment`. O(1). |
| `template <typename T> bool castle::memory::is_aligned(void const* p)` | Compile-time alignment check using `alignof(T)`. O(1). |
| `template <size_type Alignment> struct castle::memory::type_with_alignment` | Produces nested `type` aligned to `Alignment`. |
| `template <size_type Alignment> using castle::memory::type_with_alignment_t = ...` | Alias for the aligned placeholder type. |
| `template <size_type Length, size_type Alignment> struct castle::memory::aligned_storage` | Produces nested `type` containing `Length` aligned bytes plus `get_address<T>()` and `get_reference<T>()`. |
| `template <size_type Length, size_type Alignment> using castle::memory::aligned_storage_t = ...` | Alias for the concrete aligned storage object. |
| `template <size_type Length, typename T> struct castle::memory::aligned_storage_as` | Same as `aligned_storage`, but alignment comes from `alignof(T)`. |
| `template <size_type Length, typename T> using castle::memory::aligned_storage_as_t = ...` | Alias for the concrete `aligned_storage_as` object. |
| `castle::type_with_alignment*`, `castle::aligned_storage*`, `castle::aligned_storage_as*` aliases | Top-level namespace aliases for the same templates. |

## Usage Example
```cpp
// See: samples/sample_alignment.cpp
using packet_storage_t = castle::aligned_storage_as_t<sizeof(uint32_t), uint32_t>;

packet_storage_t storage;
uint32_t* ptr = storage.get_address<uint32_t>();
bool ok = castle::memory::is_aligned<uint32_t>(ptr);
```

## Constraints & Notes
- `Alignment` and `required_alignment` must be non-zero powers of two.
- `get_address<T>()` only returns a typed pointer to raw bytes; it does not construct `T`.
- `get_reference<T>()` is only valid after a live `T` object already occupies the storage.
- Callers must ensure the chosen storage size and alignment are sufficient for the object they place there.
