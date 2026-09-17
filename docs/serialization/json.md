# `castle::serialization::json`

Header-only JSON DOM, parser, and serializer for Castle's embedded profile.

## Design goals

- No heap allocation.
- No exceptions, RTTI, virtual dispatch, or STL dependency.
- Deterministic compile-time capacities.
- Parsed strings are owned by a fixed document arena.
- JSON number spelling is retained, avoiding accidental integer precision loss.
- Compact and deterministic pretty serialization.

## Include

```cpp
#include "castle/serialization/json.hpp"
```

## Document

```cpp
using json = castle::serialization::json::document<64U, 2048U, 16U>;
```

Template parameters are `MaxNodes`, `MaxStringBytes`, and `MaxDepth`.

The DOM uses fixed node storage plus one fixed character arena. Object members and array elements retain insertion order.

## Parse

```cpp
json doc;
auto result = castle::serialization::json::parse(
    doc,
    castle::container::string_view("{\"name\":\"castle\",\"enabled\":true}"));

if (result.succeeded()) {
    auto root = doc.root();
    auto name = doc.find(root, "name");
}
```

The parser accepts JSON objects, arrays, strings, numbers, `true`, `false`, and `null`. It supports the standard string escapes and four-digit `\uXXXX` BMP escapes. Surrogate code units are rejected rather than producing invalid UTF-8.

Trailing commas, comments, leading-zero numbers, `+1`, `NaN`, and `Infinity` are rejected. On failure the document is cleared. `result::offset` is zero-based; `line` and `column` are one-based.

## Construct

```cpp
json doc;
json::node_id root = json::npos;
json::node_id node = json::npos;

doc.make_object(root);
doc.set_string(node, "castle", root, "name");
doc.set_bool(node, true, root, "enabled");
doc.set_integer(node, 115200, root, "baud");
doc.make_array(node, root, "channels");
doc.set_integer(node, 1, node);
doc.set_integer(node, 2, node);
```

Available creators are `make_object`, `make_array`, `set_null`, `set_bool`, `set_string`, `set_number`, `set_integer`, `set_enum`, and `set_floating`.

## Access

Use `find(object, key)` for object members and `at(array, index)` for arrays. `scalar()` returns the stored scalar spelling and `string()` returns decoded string data.

Typed accessors are `get_bool`, `get_integer<T>`, `get_enum<T>`, `get_floating<T>`, and `get_string`.

## Serialization

Compact output:

```cpp
castle::container::string<512U> output;
auto result = castle::serialization::json::serialize(doc, output);
```

Pretty output:

```cpp
castle::container::string<1024U> output;
castle::serialization::json::serialize(doc, output, true, 2U);
```

Serialization writes directly to the caller's fixed-capacity string. On overflow it clears the output and returns `castle::status::full`.

## Embedded behavior

No dynamic allocation is used. Capacity failures are explicit `castle::status` results. Node handles are integer IDs rather than pointers, keeping the representation relocatable and deterministic.

Duplicate object keys are preserved; `find()` returns the first matching member. Applications that require duplicate-key rejection can validate their object members after parsing.

Floating-point serialization uses Castle's fixed-point `string_builder` conversion with an explicit precision. This is deterministic and libc-independent, but it is not a shortest-round-trip formatter.

## Errors

`result::status` uses Castle's common status type. `result::code` adds JSON-specific diagnostics such as `unexpected_end`, `invalid_string`, `invalid_escape`, `invalid_number`, `depth_exceeded`, `capacity`, and `output_full`.

## Example sizing

```cpp
using configuration = castle::serialization::json::document<32U, 1024U, 8U>;
```

The maximum storage requirements are explicit at compile time.
