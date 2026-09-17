# Hash

## Overview
This header provides deterministic hash functors for common embedded key types. It exists for fixed-capacity hash tables and registries that need stable hashing without the standard library.

## Header
`#include "castle/utility/hash.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [traits.md](../core/traits.md), [types.md](../core/types.md), [compare.md](compare.md), [string_view.md](../container/string_view.md)

## Public API
| API | Description |
|---|---|
| `hash_impl<T, ...>` | Helper selected internally by `hash<T>` for integral, enum, and pointer keys. |
| `hash<T>` | Default hash functor for integral, enum, and pointer types. |
| `hash<const T>` | Forwards hashing of const-qualified values to `hash<T>`. |
| `hash<container::basic_string_view<CharT>>` | Hashes every byte of a Castle string view. |

## Usage Example
See `samples/sample_hash.cpp`.

```cpp
castle::size_type id_hash = castle::hash<uint32_t>()(123U);
castle::size_type name_hash = castle::hash<castle::container::string_view>()("TEMP");
```

## Constraints & Notes
- No dynamic allocation.
- Unsupported user-defined key types require an explicit specialization.
- String-view hashing processes the entire view contents; embedded nulls are hashed if present in the view length.
