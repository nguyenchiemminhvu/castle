# Type Index

## Overview
`castle::type_index` is the counterpart of `std::type_index`: a copyable, ordered, hashable value that wraps a [`castle::type_info`](type_info.md). `type_info` objects cannot be copied, so `type_index` is how type identities are stored by value: as keys in fixed-capacity maps and hash tables, as members of registries, or as entries in sorted tables.

## Header
`#include "castle/utility/type_index.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [types.md](../core/types.md), [hash.md](hash.md), [type_info.md](type_info.md)

## Public API
| API | Description |
|---|---|
| `type_index(const type_info&)` | Implicit, `constexpr` wrapper, so `type_id<T>()` converts where a `type_index` is expected. |
| `name()` | Forwards to `type_info::name()`. |
| `hash_code()` | Forwards to `type_info::hash_code()`. |
| `operator==`, `operator!=` | Same-type comparison. |
| `operator<`, `<=`, `>`, `>=` | Ordering by `type_info::before()`. |
| `castle::hash<type_index>` | Hash specialization returning `hash_code()`, for Castle hash containers. |

## Usage Example
See [`samples/utility/type_index.cpp`](../../samples/utility/type_index.cpp).

```cpp
#include "castle/utility/type_index.hpp"

castle::type_index handlers_key[2] = { castle::type_id<uint32_t>(), castle::type_id<float>() };

castle::type_index lookup = castle::type_id<const float&>();
bool found = (lookup == handlers_key[1]);                    // true
castle::size_type bucket = castle::hash<castle::type_index>()(lookup);
```

## Constraints & Notes
- **One pointer**, trivially copyable, no heap, no exceptions, no virtuals, no RTTI.
- **No default constructor**, matching `std::type_index`. A `type_index` always refers to a type.
- **Order and hash** follow `type_info`. They are fixed within one linked image and may differ between builds, so do not persist them or rely on a particular ordering.
- Equality is a pointer compare. `castle::less<type_index>` and `castle::equal_to<type_index>` work through the operators.
