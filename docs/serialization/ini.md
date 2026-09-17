# Ini

## Overview
A fixed-capacity INI document, parser, and serializer for embedded configuration data. Use it when simple section/key/value text must be owned, queried, and emitted without heap allocation.

## Header
`#include "castle/serialization/ini.hpp"`

## Dependencies
- [`../error/status.md`](../error/status.md)
- [`../container/string.md`](../container/string.md)
- [`../container/string_view.md`](../container/string_view.md)
- [`../container/vector.md`](../container/vector.md)
- [`../utility/optional.md`](../utility/optional.md)

## Public API
| API | Description |
| --- | --- |
| `enum class error_code` | INI-specific diagnostics: `invalid_section`, `invalid_key`, `invalid_assignment`, `invalid_quote`, `invalid_escape`, `capacity`, `output_full`. |
| `struct result` | Bundles `castle::status`, `error_code`, and one-based `line` / `column` diagnostics. |
| `document<MaxEntries, MaxSectionLength, MaxKeyLength, MaxValueLength>` | Fixed-capacity owning INI document. |
| `size`, `capacity`, `empty`, `full`, `clear` | Container-style document inspection and reset. |
| `contains`, `contains_section`, `find`, `get`, `read` | Lookup helpers for global or section-scoped keys. |
| `set`, `set_bool`, `set_integer`, `set_enum` | Insert or update values as text. |
| `get_bool`, `get_integer`, `get_enum`, `get_floating` | Read typed values from stored text. |
| `remove`, `remove_section` | Delete one key or an entire section. |
| `section`, `key`, `value` | Indexed access to stored entries in insertion order. |
| `parse` | Parses INI text into a document and clears the document on failure. |
| `serialize`, `write` | Deterministically emits INI text into a caller-owned fixed string. |

## Usage Example
```cpp
// See: samples/sample_ini.cpp
#include "castle/serialization/ini.hpp"

using config_t = castle::serialization::ini::document<16U, 24U, 32U, 96U>;
config_t config;
```

## Constraints & Notes
- No heap allocation; entries are stored in fixed-capacity Castle strings and a fixed-capacity vector.
- Supported syntax: sections (`[name]`), global keys, `=` or `:` assignment, leading/trailing whitespace trimming, full-line `#` / `;` comments, trailing comments after section headers, single- or double-quoted values, and escapes `\\`, `\"`, `\n`, `\r`, `\t`.
- Unquoted values are preserved verbatim after trimming; inline comments are not stripped from them.
- Boolean reads accept `true/false`, `yes/no`, `on/off`, and `1/0` case-insensitively.
- Integer and floating-point parsing perform overflow/range checks and report failures through `castle::status` plus `error_code`.
