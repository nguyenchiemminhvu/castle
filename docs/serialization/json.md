# Json

## Overview
A fixed-capacity JSON DOM, parser, and serializer designed for Castle's deterministic embedded profile. Use it when JSON objects and arrays must be parsed, built, traversed, and serialized without heap allocation.

## Header
`#include "castle/serialization/json.hpp"`

## Dependencies
- [`../error/status.md`](../error/status.md)
- [`../container/array.md`](../container/array.md)
- [`../container/string.md`](../container/string.md)
- [`../container/string_view.md`](../container/string_view.md)
- [`../utility/string_builder.md`](../utility/string_builder.md)

## Public API
| API | Description |
| --- | --- |
| `enum class type` | JSON node kinds: `null_value`, `boolean`, `number`, `string`, `object`, `array`. |
| `enum class error_code` | Parse/serialization diagnostics such as `unexpected_end`, `invalid_string`, `invalid_number`, `depth_exceeded`, `capacity`, and `output_full`. |
| `struct result` | Bundles `castle::status`, `error_code`, zero-based `offset`, and one-based `line` / `column`. |
| `document<MaxNodes, MaxStringBytes, MaxDepth>` | Fixed-capacity owning DOM with node storage and a shared string arena. |
| `root`, `kind`, `parent`, `first_child`, `next_sibling`, `key`, `scalar`, `string` | Node inspection and traversal helpers. |
| `find`, `at` | Object-member and array-element lookup helpers. |
| `set_null`, `set_bool`, `set_string`, `set_number`, `set_integer`, `set_enum`, `set_floating` | Construct scalar nodes. |
| `make_object`, `make_array`, `append` | Construct containers and attach child nodes. |
| `get_bool`, `get_string`, `get_integer`, `get_enum`, `get_floating` | Typed value extraction from scalar nodes. |
| `parse` | Parses JSON text into a document and clears the document on failure. |
| `serialize` | Emits compact or pretty JSON into a caller-owned fixed string. |

## Usage Example
```cpp
// See: samples/sample_json.cpp
#include "castle/serialization/json.hpp"

using json_document = castle::serialization::json::document<32U, 512U, 8U>;
json_document doc;
```

## Constraints & Notes
- No heap allocation; nodes and all stored strings live in fixed-size arrays.
- Supported syntax: objects, arrays, strings, numbers, `true`, `false`, and `null`.
- Rejected syntax/features: comments, trailing commas, `+1`, leading-zero integers, `NaN`, and `Infinity`.
- Strings support standard JSON escapes plus `\uXXXX` BMP escapes; surrogate code units are rejected.
- Number nodes preserve their original textual spelling in the DOM; typed conversions are explicit through getters.
