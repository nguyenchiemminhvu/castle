# String Builder

## Overview
`castle::basic_string_builder` builds bounded diagnostic strings in place using a fixed-capacity Castle string. It exists for logging, telemetry, and formatting on embedded systems that forbid heap allocation and iostream-style dependencies.

## Header
`#include "castle/utility/string_builder.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [config.md](../core/config.md), [types.md](../core/types.md), [traits.md](../core/traits.md), [status.md](../error/status.md), [string.md](../container/string.md), [string_view.md](../container/string_view.md)

## Public API
| API | Description |
|---|---|
| `basic_string_builder<CharT, N, TrimWhitespaceBeforeEllipsis>` | Fixed-capacity builder for `char` or `wchar_t`. |
| `append(value)` / `append(value, precision)` | Appends supported values, with an overload for explicit floating-point precision. |
| `operator<<(value)` | Streaming-style append syntax. |
| `build(args...)` | Appends a heterogeneous pack in order. |
| `clear()` | Resets the builder and clears the truncation state. |
| `size()`, `capacity()`, `empty()`, `full()` | Capacity and occupancy queries. |
| `truncated()`, `build_status()` | Reports whether overflow sealing with `...` has occurred. |
| `c_str()`, `view()` | Exposes the current built text. |
| `string_builder<N, TrimWhitespaceBeforeEllipsis>` | `char` builder alias. |
| `wstring_builder<N, TrimWhitespaceBeforeEllipsis>` | `wchar_t` builder alias. |

## Usage Example
See `samples/sample_string_builder.cpp`.

```cpp
castle::string_builder<32U> line;
line.build("temp=", 25.3F, " ok=", true);
```

## Constraints & Notes
- Fixed capacity `N`; no dynamic allocation.
- Once truncation occurs, the builder seals itself with `...` and ignores later appends.
- Floating-point formatting is dependency-free and uses a bounded decimal precision.
- Only `char` and `wchar_t` builders are supported.
