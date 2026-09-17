# Xml

## Overview
A fixed-capacity XML DOM, parser, and serializer for embedded configuration and protocol data. Use it when well-formed XML must be parsed, traversed, modified, or emitted without heap allocation.

## Header
`#include "castle/serialization/xml.hpp"`

## Dependencies
- [`../error/status.md`](../error/status.md)
- [`../container/array.md`](../container/array.md)
- [`../container/string.md`](../container/string.md)
- [`../container/string_view.md`](../container/string_view.md)
- [`../utility/string_builder.md`](../utility/string_builder.md)

## Public API
| API | Description |
| --- | --- |
| `enum class type` | XML node kinds: `document`, `element`, `text`, `cdata`, `comment`, `processing_instruction`, `declaration`, `doctype`. |
| `enum class error_code` | XML diagnostics such as `malformed_name`, `mismatched_tag`, `invalid_entity`, `invalid_comment`, `depth_exceeded`, `capacity`, and `output_full`. |
| `struct result` | Bundles `castle::status`, `error_code`, zero-based `offset`, and one-based `line` / `column`. |
| `document<MaxNodes, MaxAttributes, MaxStringBytes, MaxDepth>` | Fixed-capacity owning XML DOM with node storage, attribute storage, and a shared string arena. |
| `document_node`, `root`, `kind`, `parent`, `first_child`, `next_sibling` | DOM traversal entry points. |
| `name`, `value`, `first_attribute`, `next_attribute`, `attribute_name`, `attribute_value` | Access raw node and attribute text. |
| `find_child`, `find_attribute` | Lookup helpers for elements and attributes. |
| `make_element`, `append_text`, `append_cdata`, `append_comment`, `append_processing_instruction`, `append_declaration`, `append_doctype` | Build XML structure incrementally. |
| `add_attribute`, `set_attribute`, `set_text` | Add or update attributes and element text. |
| `get_attribute`, `get_text` | Retrieve raw, boolean, integer, floating-point, or enum values. |
| `parse` | Parses XML text into a document and clears the document on failure. |
| `serialize` | Emits compact or pretty XML into a caller-owned fixed string. |

## Usage Example
```cpp
// See: samples/sample_xml.cpp
#include "castle/serialization/xml.hpp"

using xml_document = castle::serialization::xml::document<32U, 32U, 1024U, 8U>;
xml_document doc;
```

## Constraints & Notes
- No heap allocation; nodes, attributes, and all stored text use fixed-capacity inline storage.
- Supported subset: UTF-8 text, one XML declaration, one DOCTYPE, elements, attributes, comments, CDATA, processing instructions, predefined entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`), and numeric character references.
- Element and attribute names use the ASCII subset implemented by the parser: letters, digits, `_`, `:`, `-`, and `.` with a letter/`_`/`:` start.
- DOCTYPE text is preserved but not validated or expanded.
- `get_text()` returns `castle::status::data_loss` when an element has multiple direct text/CDATA children.
