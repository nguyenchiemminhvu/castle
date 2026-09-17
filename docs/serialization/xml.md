# Castle XML serialization

## Overview

`castle::serialization::xml` is a fixed-capacity, header-only XML 1.x DOM/parser/serializer intended for embedded systems.

The module follows Castle's core constraints:

- no heap allocation;
- no exceptions;
- no RTTI;
- no virtual functions;
- no STL dependency;
- deterministic storage and capacity failure;
- caller-selected compile-time limits.

Header:

```cpp
#include "castle/serialization/xml.hpp"
```

Namespace:

```cpp
castle::serialization::xml
```

The DOM owns all parsed names, text and attribute values in a fixed character arena. Tree and attribute links are fixed arrays. The parser itself uses a fixed compile-time element stack rather than recursive descent, so parser nesting is bounded by `MaxDepth`.

## XML profile

The implementation targets a practical, well-formed XML profile suitable for embedded configuration and protocol data.

Supported:

- one document element;
- nested elements and self-closing elements;
- attributes with single or double quotes;
- UTF-8 text;
- BOM at the beginning of an input document;
- predefined entities `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`;
- numeric character references in decimal and hexadecimal form;
- XML comments;
- CDATA sections;
- processing instructions;
- XML declarations for versions `1.0` and `1.1`;
- DOCTYPE text capture, including an internal subset, without DTD/entity expansion;
- deterministic pretty and compact serialization.

Embedded-profile limitations are intentional:

- element/attribute/PI names use an ASCII XML-name subset: letters, `_`, `:`, digits, `-`, and `.`;
- namespace prefixes such as `cfg:device` are preserved as names, but namespace URI resolution is not performed;
- DTD declarations are captured as text but are not validated or expanded;
- only the five predefined entities and numeric character references are resolved;
- insignificant whitespace outside the document element is not retained as text nodes;
- this is a well-formedness parser, not a schema/DTD validating parser.

This keeps memory use deterministic and avoids dynamic entity tables, external-resource loading, recursive DTD expansion, and other behaviors unsuitable for firmware.

## Document type

The main container is:

```cpp
template <
    castle::size_type MaxNodes = 128U,
    castle::size_type MaxAttributes = 128U,
    castle::size_type MaxStringBytes = 4096U,
    castle::size_type MaxDepth = 16U>
class document;
```

Example:

```cpp
using config_document =
    castle::serialization::xml::document<64U, 64U, 2048U, 8U>;
```

The configured values are compile-time contracts:

| Parameter | Meaning |
|---|---|
| `MaxNodes` | Maximum number of user-visible DOM nodes. The internal document node is not counted by `size()`. |
| `MaxAttributes` | Maximum total number of attributes across the document. |
| `MaxStringBytes` | Shared storage for element names, PI targets, text, comments, CDATA, declaration/DOCTYPE text, attribute names and values. |
| `MaxDepth` | Maximum nested element depth. |

The document has an internal synthetic document node returned by `document_node()`. `root()` returns the first element node.

## Parsing

Parse into an existing document:

```cpp
using namespace castle::serialization::xml;

config_document doc;

const result r = parse(
    doc,
    castle::container::string_view(
        "<?xml version=\"1.0\"?>"
        "<device id=\"42\" enabled=\"true\">"
            "<rate>1000</rate>"
        "</device>"));

if (r.succeeded())
{
    const config_document::node_id root = doc.root();
    // use the DOM...
}
```

`document::parse()` is also available when only `castle::status` is needed:

```cpp
if (doc.parse(source) != castle::status::ok)
{
    // parse failed
}
```

The full `result` exposes:

```cpp
struct result
{
    castle::status status;
    error_code code;
    castle::size_type offset;
    castle::size_type line;
    castle::size_type column;
};
```

A failed parse clears the document so callers never receive a partially valid DOM.

## DOM traversal

Basic inspection:

```cpp
const auto root = doc.root();

castle::container::string_view root_name = doc.name(root);
auto child = doc.first_child(root);

while (child != config_document::npos)
{
    if (doc.kind(child) == type::element)
    {
        // process child
    }
    child = doc.next_sibling(child);
}
```

Useful lookups:

```cpp
auto rate = doc.find_child(root, "rate");
auto attr = doc.find_attribute(root, "enabled");
```

The DOM intentionally preserves insertion order for both children and attributes.

## Attributes

Read an attribute without allocating:

```cpp
castle::container::string_view id_text;
if (doc.get_attribute(root, "id", id_text) == castle::status::ok)
{
    // id_text refers to the document's fixed arena
}
```

Typed access is available for integral, enum, floating-point and boolean values:

```cpp
uint32_t id = 0U;
bool enabled = false;
double gain = 0.0;

const auto id_status = doc.get_attribute(root, "id", id);
const auto enabled_status = doc.get_attribute(root, "enabled", enabled);
const auto gain_status = doc.get_attribute(root, "gain", gain);
```

Boolean text accepts `true`, `false`, `1`, and `0`.

Mutation:

```cpp
doc.set_attribute(root, "id", 43U);
doc.set_attribute(root, "enabled", true);
doc.set_attribute(root, "gain", 2.5, 3U);
```

Adding a duplicate attribute with `add_attribute()` returns `castle::status::already_exists`. `set_attribute()` updates an existing attribute or creates it when absent. Because the string arena is append-only, replacing an existing value consumes additional arena bytes; repeated updates should therefore be bounded or followed by rebuilding/clearing the document.

## Elements and text

Create a document:

```cpp
config_document doc;
config_document::node_id root = config_document::npos;
config_document::node_id rate = config_document::npos;

doc.make_element(root, "device");
doc.make_element(rate, "rate", root);
doc.set_text(rate, 1000U);
```

Text can also be added explicitly:

```cpp
config_document::node_id text = config_document::npos;
doc.append_text(text, "castle", root);
```

`set_text()` replaces the first direct text child when one exists; otherwise it creates one. Replacing text also consumes new arena bytes for the same append-only reason. `get_text()` is deliberately conservative: if an element contains more than one direct text/CDATA node it returns `castle::status::data_loss` rather than silently concatenating unrelated content.

## CDATA, comments and processing instructions

```cpp
config_document::node_id cdata = config_document::npos;
config_document::node_id comment = config_document::npos;
config_document::node_id pi = config_document::npos;

doc.append_cdata(cdata, "raw <xml> & data", root);
doc.append_comment(comment, "diagnostic", root);
doc.append_processing_instruction(pi, "mode", "debug", root);
```

`append_cdata()` rejects `]]>` because that sequence terminates a CDATA section.

`append_comment()` rejects `--` and values ending with `-`, matching XML comment constraints.

## XML declaration and DOCTYPE

Top-level declaration and DOCTYPE nodes can be created explicitly:

```cpp
config_document::node_id declaration = config_document::npos;
config_document::node_id doctype = config_document::npos;

doc.append_declaration(declaration, "version=\"1.0\" encoding=\"UTF-8\"");
doc.append_doctype(doctype, " root");
```

The parser accepts XML declarations for versions `1.0` and `1.1`, and recognizes the `encoding` and `standalone` fields. DTD text is retained but does not create or execute an entity table.

## Serialization

Serialize into a caller-owned fixed string:

```cpp
castle::container::string<1024U> output;

const result r = serialize(doc, output);
```

Pretty output:

```cpp
serialize(doc, output, true, 2U);
```

The serializer uses `2U` spaces by convention in samples, but the indentation is caller-selectable.

Output overflow is deterministic. The output is cleared and the result is:

```cpp
status == castle::status::full
code   == error_code::output_full
```

Pretty-printing does not inject formatting whitespace into mixed content such as:

```xml
<root>Hello <b>world</b>!</root>
```

The text remains semantically unchanged.

## Error model

`error_code` provides parser-specific diagnostics on top of Castle's existing `castle::status` abstraction.

| Code | Meaning |
|---|---|
| `unexpected_end` | Input ended before a required construct completed. |
| `unexpected_token` | Token is not legal in the current document position. |
| `malformed_name` | Invalid element/attribute/PI name. |
| `mismatched_tag` | End tag does not match the current element. |
| `duplicate_attribute` | An element contains the same attribute name twice. |
| `invalid_attribute` | Attribute syntax is malformed. |
| `invalid_entity` | Entity or character reference is not supported or valid. |
| `invalid_character` | Input contains an invalid XML character or malformed UTF-8. |
| `invalid_comment` | XML comment constraints are violated. |
| `invalid_cdata` | CDATA content is malformed. |
| `invalid_processing_instruction` | PI syntax is malformed. |
| `invalid_declaration` | XML declaration is malformed or in the wrong position. |
| `invalid_doctype` | DOCTYPE is malformed or in the wrong position. |
| `unsupported_construct` | Reserved for constructs intentionally outside the embedded profile. |
| `depth_exceeded` | Nesting exceeds `MaxDepth`. |
| `capacity` | DOM node/attribute/string capacity was exceeded. |
| `output_full` | Serialization output buffer was too small. |

`offset` is a byte offset into the input and `line`/`column` are one-based diagnostics.

## Deterministic memory behavior

The implementation has no hidden allocation path.

For a configured document, memory is bounded by:

```text
(MaxNodes + 1) * sizeof(document::node)
+ MaxAttributes * sizeof(document::attribute)
+ MaxStringBytes
+ MaxDepth * sizeof(node_id)
```

The extra node is the internal synthetic document node.

There is no ownership of the caller's input string. Parsed names and values are copied into the fixed arena, so the input buffer may be released after `parse()` returns.

## Embedded usage guidance

Choose capacities from the protocol/configuration contract instead of treating them as implementation details. A typical firmware profile might be:

```cpp
using xml_document = castle::serialization::xml::document<
    48U,   // nodes
    64U,   // attributes
    2048U, // string arena
    8U     // element nesting
>;
```

For small configuration documents, increasing `MaxStringBytes` is usually cheaper than introducing separate dynamic string objects because all names and values already share one deterministic arena.

## Relationship to other Castle serialization formats

The XML API intentionally follows the same broad shape as Castle's JSON/INI modules:

- fixed-capacity document object;
- `castle::status` for operational outcomes;
- format-specific `result`/`error_code` for diagnostics;
- `string_view` for zero-copy inspection;
- typed accessors built on Castle traits and `string_builder`;
- explicit serialization into caller-owned fixed strings.
