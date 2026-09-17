# Variant

## Overview
`castle::variant` is a fixed-size discriminated union for embedded systems. It exists when one object must hold one of several alternative types while keeping storage, lifetime management, and dispatch explicit and allocation-free.

## Header
`#include "castle/utility/variant.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [error_handler.md](../core/error_handler.md), [types.md](../core/types.md), [type_ranges.md](../core/type_ranges.md), [traits.md](../core/traits.md), [construct.md](../memory/construct.md), [destroy.md](../memory/destroy.md), [alignment.md](../memory/alignment.md), [forward.md](forward.md), [move.md](move.md), [swap.md](swap.md)

## Public API
| API | Description |
|---|---|
| `in_place_index_t<Index>`, `in_place_index<Index>` | Tag type and value for index-based in-place construction. |
| `variant_npos` | Sentinel index used when no alternative is active. |
| `variant<Ts...>` | Fixed-size variant storing exactly one of `Ts...`, or no value after `reset()`. |
| `variant::index()`, `valueless_by_exception()` | Query the active alternative index or valueless state. |
| `variant::reset()`, `emplace<Index>(...)`, `emplace<T>(...)`, `swap(...)` | Manage the active alternative. |
| `variant::is_supported_type<T>()` | Checks whether `T` is one of the alternatives. |
| `variant_alternative<Index, Variant>`, `variant_alternative_t<Index, Variant>` | Maps an index to an alternative type. |
| `variant_size<Variant>`, `variant_size_v<Variant>` | Compile-time number of alternatives. |
| `holds_alternative<T>(variant)` / `holds_alternative<Index>(variant)` | Checks the active alternative. |
| `get<Index>(variant)` / `get<T>(variant)` | Returns the active alternative with Castle assertion checking. |
| `get_if<Index>(&variant)` / `get_if<T>(&variant)` | Returns a pointer to the active alternative or `nullptr`. |
| `visit(visitor, variant)` | Dispatches a visitor to the active alternative. |
| `swap(variant&, variant&)` | Non-member swap wrapper. |
| `make_variant<T>(args...)` | Creates a single-alternative `variant<T>` in place. |

## Usage Example
See `samples/sample_variant.cpp`.

```cpp
castle::variant<uint32_t, uint16_t> value(castle::in_place_index<0>, 77U);
uint32_t current = castle::get<uint32_t>(value);
```

## Constraints & Notes
- No heap allocation.
- Alternative types must be unique, non-array, destructible object types.
- `visit` expands dispatch as a compile-time linear chain rather than a table.
- `reset()` is a Castle-specific extension that explicitly clears the active value.
