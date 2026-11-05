# Type Info

## Overview
`castle::type_info` is the RTTI-free counterpart of `std::type_info`, and `castle::type_id<T>()` is the counterpart of `typeid(T)`. Code built with `-fno-rtti` can use them to identify, compare, order, hash, and name types at runtime, for example in type-erased containers ([`any`](any.md)), registries, and message routing. Every type has exactly one statically allocated `type_info` object. Its address is the type's identity, so an equality check is a single pointer compare. Nothing allocates, throws, uses virtual dispatch, or runs at start-up.

## Header
`#include "castle/utility/type_info.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [traits.md](../core/traits.md), [types.md](../core/types.md), [hash.md](hash.md)

## Public API
| API | Description |
|---|---|
| `type_id<T>()` | Returns `const type_info&` for `T`. References and top-level cv-qualifiers are ignored, as with `typeid`. `constexpr`, `noexcept`. |
| `type_info` | Per-type descriptor. Users cannot construct, copy, or assign it. |
| `type_info::operator==`, `operator!=` | Same-type check: a single address compare. |
| `type_info::before(other)` | Implementation-defined strict total order, by address. |
| `type_info::hash_code()` | `size_type` hash, equal for equal types. Computed with `castle::hash` on the object's address. |
| `type_info::name()` | Null-terminated, implementation-defined type name (e.g. `"unsigned int"`), or `""` when names are disabled. |

## Usage Example
See [`samples/utility/type_info.cpp`](../../samples/utility/type_info.cpp).

```cpp
#include "castle/utility/type_info.hpp"

const castle::type_info& info = castle::type_id<uint16_t>();

bool same    = (info == castle::type_id<const uint16_t&>());   // true: cv/ref ignored
bool ordered = info.before(castle::type_id<float>()) || castle::type_id<float>().before(info);
castle::size_type bucket = info.hash_code();
const char* text = info.name();                                 // "short unsigned int" on GCC
```

## Constraints & Notes
- **No RTTI, no heap, no virtuals.** Each `type_info` is one pointer, the name. `type_id<T>()` returns a reference to a static object.
- **Deterministic start-up.** The per-type object has a `constexpr` constructor, so it is constant-initialized: no static-initialization-order issues and no start-up code.
- **Identity cannot be folded.** The per-type objects are writable statics. Linkers never fold writable data, so two types never share an identity, even with `--icf=all` and with names disabled.
- **Stability.** Identity, `before()` order, and `hash_code()` are fixed within one linked image but may change between builds. Never serialize them or send them between processors. Across shared-library boundaries, identity holds only when the per-type inline variables are merged (default visibility).
- **Names.** Names are extracted at compile time from the compiler's own signature string (`__PRETTY_FUNCTION__` on GCC/Clang/armclang, `__FUNCSIG__` on MSVC). Only the extracted name, one null-terminated string, is emitted, and only for types whose `type_info` is used. Spelling follows the compiler, e.g. GCC writes `long unsigned int` and Clang `unsigned long`. Treat names as diagnostics, not keys. On unknown compilers names default to off; an unrecognized signature format triggers a `static_assert` asking you to set `CASTLE_TYPE_INFO_NAMES` to `0`.
- **Compile time vs runtime.** `type_id<T>()` is `constexpr`: binding a `const type_info&` happens at compile time. `==`, `before()`, `hash_code()`, and `name()` run at runtime. The per-type object is deliberately non-const, and compilers do not evaluate comparisons between addresses of distinct inline variables in constant expressions.
- **Thread safety.** All objects are immutable after constant initialization, so they are safe to use from any thread or ISR.
- Use [`type_index`](type_index.md) to store type identities by value or as container keys.
