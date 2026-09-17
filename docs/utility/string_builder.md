# String builder

## Overview

Fixed-capacity, heap-free string builder. It folds a heterogeneous, fully type-checked set of values — characters, booleans, integers, floating-point numbers, enums, C strings/views, or any Castle container exposing `view()` — into an internal `castle::container::basic_string<CharT, N>`. Overflow is deterministic and one-shot: the first value that does not fully fit is truncated to whatever space remains and the builder is permanently sealed with a trailing `"..."` marker, so a diagnostic line never exceeds its declared capacity and callers never need to pre-compute a total length.

## Header

```cpp
#include "castle/utility/string_builder.hpp"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `basic_string_builder<CharT, N, TrimWhitespaceBeforeEllipsis = true>` | Builder over a fixed `N`-character buffer of `CharT`; the trim flag is a compile-time policy, not a runtime option. |
| `string_builder<N, Trim = true>` / `wstring_builder<N, Trim = true>` | Convenience aliases for `char` and `wchar_t` builders. |
| `append(value)` | Type-safe dispatch for `CharT`, `bool`, integral, floating-point (default precision), enums (scoped and unscoped), types convertible to `basic_string_view<CharT>`, and any type exposing `view()`; unsupported types fail at compile time. |
| `append(value, precision)` | Floating-point overload with an explicit decimal precision, clamped to `max_float_precision`. |
| `operator<<` | Same dispatch as `append()`, for iostream-style chaining. |
| `build(args...)` | Appends every argument in order; equivalent to one `append()` call per argument. |
| `truncated()` / `build_status()` | Whether the overflow marker has been applied, as a `bool` or a `castle::status` (`ok` / `data_loss`). |
| `clear()` | Empties the buffer and clears the truncated/sealed state. |
| `size()` / `capacity()` / `empty()` / `full()` | Buffer state. |
| `c_str()` / `view()` | Null-terminated access or a `basic_string_view<CharT>` over the current content. |

## Example

```cpp
castle::string_builder<64U> line;
line.build("temp=", 25.3F, "C state=", link_state::up, " ok=", true);
// line.view() == "temp=25.300C state=1 ok=true"

castle::string_builder<32U> chained;
chained << "x=" << 10 << " y=" << -20;
// chained.view() == "x=10 y=-20"

castle::string_builder<32U> chained_append;
chained_append.append("a").append(" ").append("b");
// chained_append.view() == "a b"

castle::string_builder<12U> short_line;
short_line.append("this line is definitely too long");
// short_line.truncated() == true, short_line.view() == "this line..."
```

## Embedded notes

- No heap allocation, exceptions, RTTI, or virtual dispatch; overflow is reported through `truncated()` / `build_status()`, not exceptions.
- Once sealed by overflow, the builder ignores further `append()`/`build()` calls until `clear()` is called; content never grows past `N` characters.
- `TrimWhitespaceBeforeEllipsis` (default `true`) strips trailing whitespace already written before placing the `"..."` marker, so padded fields do not end up with dangling spaces before it; set it to `false` to keep the marker glued to whatever was written, whitespace included. Being a template parameter, the choice is resolved at compile time with no runtime branch.
- Floating-point precision defaults to `default_float_precision` (3) and is clamped to `max_float_precision` (9); `nan`/`inf` are reported as literal text rather than computed via `<math.h>`.
- Scoped and unscoped enums are appended via their underlying integral type.
- Only `char` and `wchar_t` are supported `CharT` instantiations; anything else fails the class-level `static_assert`.
