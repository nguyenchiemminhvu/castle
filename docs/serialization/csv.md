# Csv

## Overview
A fixed-capacity CSV document, parser, and serializer for embedded configuration and tabular data. Use it when CSV text must be parsed, traversed, modified, or emitted with deterministic memory usage and without heap allocation.

## Header
`#include "castle/serialization/csv.hpp"`

## Dependencies
- [`../error/status.md`](../error/status.md)
- [`../container/array.md`](../container/array.md)
- [`../container/string.md`](../container/string.md)
- [`/container/string_view.md`](/container/string_view.md)

## Public API

| API | Description |
| --- | --- |
| `enum class error_code` | CSV diagnostics such as `unexpected_end`, `invalid_quote`, `invalid_delimiter`, `inconsistent_columns`, `capacity`, and `output_full`. |
| `struct result` | Bundles `castle::status`, `error_code`, zero-based `offset`, and one-based `line` / `column`. |
| `struct options` | Parser and serializer configuration containing field delimiter and strict-column validation settings. |
| `document<MaxRows, MaxColumns, MaxFieldLength>` | Fixed-capacity CSV document with owning storage for rows and fields. |
| `size`, `rows`, `capacity`, `empty`, `full`, `clear` | Document capacity and state management utilities. |
| `add_row` | Appends a new empty row and returns its index. |
| `append`, `set`, `append_character` | Add or modify field values. |
| `read`, `get` | Retrieve field values as string views. |
| `column_count`, `max_column_count`, `row_full` | Row and column inspection helpers. |
| `set_bool`, `set_integer`, `set_enum`, `set_floating` | Store typed values as CSV text. |
| `get_bool`, `get_integer`, `get_enum`, `get_floating` | Read and convert field values to typed data. |
| `parse` | Parses CSV text into a document and clears the document on failure. |
| `serialize` | Serializes a document to CSV text using configurable delimiters and automatic field escaping. |
| `write` | Convenience wrapper around `serialize`. |

## Usage Example

```cpp
// See: samples/sample_csv.cpp
#include "castle/serialization/csv.hpp"

using csv_document =
    castle::serialization::csv::document<32U, 16U, 128U>;

csv_document doc;
```

## Constraints & Notes

- No heap allocation; all rows, columns, and field storage use fixed-capacity inline containers.
- Capacity limits are specified at compile time through `MaxRows`, `MaxColumns`, and `MaxFieldLength`.
- All field values are owned by the document and remain valid until modified or cleared.
- Supports configurable field delimiters.
- Supports quoted fields, escaped quotes (`""`), embedded delimiters, and embedded line breaks inside quoted fields.
- Supports optional UTF-8 BOM removal during parsing.
- Both CRLF (`\r\n`) and LF (`\n`) record terminators are accepted during parsing.
- Serialized output always uses LF (`\n`) line endings.
- Fields are automatically quoted during serialization when they contain the delimiter, double quotes, carriage returns, or line feeds.
- Embedded double quotes are escaped as two consecutive double quote characters.
- When `options::strict_columns` is enabled, every record must contain the same number of columns as the first record.
- Empty input is valid and produces an empty document.
- Typed accessors support:
  - Boolean values: `"true"`, `"false"`, `"1"`, `"0"` (case-insensitive for text values).
  - Signed and unsigned integer values.
  - Enumeration values through their underlying integer representation.
  - Floating-point values including decimal and scientific notation.
- Field values must not contain embedded null (`'\0'`) characters.
