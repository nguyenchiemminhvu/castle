# `castle::serialization::ini`

`castle::serialization::ini` is the fixed-capacity INI data format for Castle.
It is designed for the same embedded constraints as the rest of the library:

- no heap allocation
- no exceptions
- no RTTI
- no virtual functions
- no STL containers or algorithms
- deterministic storage and iteration order

The implementation is header-only and depends only on existing Castle
utilities: `castle::status`, `castle::container::string`,
`castle::container::string_view`, `castle::container::vector`,
`castle::optional`, and Castle type traits.

## Header

```cpp
#include "castle/serialization/ini.hpp"
```

## Design

The main type is:

```cpp
template <castle::size_type MaxEntries = 32U,
          castle::size_type MaxSectionLength = 32U,
          castle::size_type MaxKeyLength = 32U,
          castle::size_type MaxValueLength = 128U>
class castle::serialization::ini::document;
```

Each entry owns its section, key, and value in fixed-capacity Castle strings.
The parsed input buffer therefore does not need to stay alive after `parse()`
returns.

The document keeps entries in insertion order. Assigning the same
`section + key` again updates the existing entry in place rather than creating
a duplicate entry. Removing an entry preserves the relative order of the
remaining entries.

`parse()` clears the destination before parsing. When a parse error occurs it
clears it again, so a failed parse never exposes a partially parsed
configuration.

Every `castle::status` / `castle::optional` / `result` returning member and
free function (`set*`, `get*`, `read`, `remove*`, `contains*`, `find`, `get`,
`parse`, `serialize`, `write`) is `[[nodiscard]]`. A capacity, lookup, or
parse failure that is silently dropped is exactly the class of defect a
safety-critical caller must not allow, so the compiler rejects code that
ignores the result.

## Parsing

Use `castle::serialization::ini::parse()` with a `castle::container::string_view`:

```cpp
using config_t = castle::serialization::ini::document<16U, 24U, 32U, 96U>;

config_t config;
const castle::container::string_view text(
    "# device configuration\n"
    "device = castle-demo\n"
    "[network]\n"
    "enabled = yes\n"
    "port = 1883\n"
    "host = \"broker.example.com\"\n");

const castle::serialization::ini::result result =
    castle::serialization::ini::parse(config, text);

if (!result.succeeded())
{
    // result.status, result.code, result.line, result.column
}
```

Supported input forms:

```ini
# full-line comments
; another comment

name = castle
port: 1883

[network]
enabled = yes
host = "broker.example.com"
message = "line1\nline2"
```

The parser accepts `=` and `:` as key/value separators. Leading and trailing
ASCII whitespace around sections, keys, and values is ignored.

Comments are recognized when `#` or `;` is the first non-whitespace character
of a line. They are not stripped from unquoted values, so a value such as
`value#suffix` remains exactly that string.

Section headers may have a trailing comment:

```ini
[network] ; comment
```

CR/LF, LF, and CR line endings are accepted.

## Quoted values

Values may be unquoted or quoted with either `"` or `'`.
Double-quoted values are the canonical form emitted by the serializer when a
value needs quoting.

Supported escapes inside quoted values are:

| Escape | Stored character |
| --- | --- |
| `\\"` | `"` |
| `\\\\` | `\\` |
| `\\n` | newline |
| `\\r` | carriage return |
| `\\t` | tab |

An unknown escape sequence is rejected with `error_code::invalid_escape`.
An unterminated or otherwise malformed quoted value is rejected with
`error_code::invalid_quote`.

## Lookup and modification

For a no-throw optional lookup:

```cpp
auto port_text = config.get("network", "port");
if (port_text.has_value())
{
    // port_text.value() is a castle::container::string_view.
}
```

For callers that prefer an explicit status:

```cpp
castle::container::string_view value;
if (config.read("network", "port", value) == castle::status::ok)
{
    // use value
}
```

Global-scope keys omit the section argument:

```cpp
config.set("device", "castle-demo");
config.set("network", "port", "1883");
```

The following modification/query operations are available:

- `set(section, key, value)`
- `set(key, value)`
- `contains(section, key)` / `contains(key)`
- `contains_section(section)`
- `get(section, key)` / `get(key)` returning `castle::optional<string_view>`
- `read(section, key, out)` / `read(key, out)` returning `castle::status`
- `remove(section, key)` / `remove(key)`
- `remove_section(section)`
- `clear()`

Values returned by `get()` and `read()` are views into the document. They
remain valid until that entry is modified or the document is cleared/moved.

## Typed helpers

Boolean helpers accept the following spellings case-insensitively:

- true: `true`, `yes`, `on`, `1`
- false: `false`, `no`, `off`, `0`

Example:

```cpp
bool enabled = false;
if (config.get_bool("network", "enabled", enabled) == castle::status::ok)
{
    // use enabled
}
```

Integral values are parsed as signed/unsigned decimal integers with overflow
checking. Enum helpers use Castle's `underlying_type` trait.

```cpp
uint16_t port = 0U;
castle::status status = config.get_integer("network", "port", port);

mode current = mode::safe;
status = config.get_enum("device", "mode", current);
```

Floating-point reads accept decimal notation with an optional exponent, for
example `-1.25`, `3.125e+1`, and `4e-2`. Exponents larger than 1024 in absolute
value are rejected to keep parsing time bounded. Values exceeding the target
floating-point type's maximum are reported as `castle::status::out_of_range`.

Castle intentionally stores INI values as text rather than inferring a
runtime variant type. This keeps the document small, predictable, and easy to
map onto strongly typed firmware configuration structures.

## Serialization

Serialize to any fixed-capacity `castle::container::string<N>`:

```cpp
castle::container::string<512U> text;
const castle::serialization::ini::result result =
    castle::serialization::ini::serialize(config, text);
```

`write()` is an alias of `serialize()`.

Serialization is deterministic:

1. global keys are emitted first;
2. sections are emitted in first-seen order;
3. entries inside a section retain insertion order;
4. each assignment uses `key = value`;
5. values are quoted and escaped only when required for round-trip safety;
6. section/key names are emitted exactly as stored.

The output always ends each assignment and section with a newline. A blank
line is emitted after the global block and after each section.

## Error reporting

Generic operation failures use existing `castle::status` values. The INI
specific `error_code` adds enough information to diagnose parse and output
failures without expanding Castle's global status enumeration.

`result` contains:

```cpp
struct result
{
    castle::status status;
    error_code code;
    castle::size_type line;
    castle::size_type column;
};
```

Line and column are one-based for parse errors. Serialization failures have
line and column set to zero.

The principal parse error codes are:

| Code | Meaning |
| --- | --- |
| `invalid_section` | malformed or empty section header |
| `invalid_key` | malformed or empty key |
| `invalid_assignment` | missing `=`/`:` separator or invalid assignment |
| `invalid_quote` | malformed quoted value |
| `invalid_escape` | unsupported escape sequence |
| `capacity` | fixed-capacity document/string limit reached |
| `output_full` | serialization output string is too small |

Typical generic statuses are:

| Status | Typical INI meaning |
| --- | --- |
| `ok` | operation completed |
| `full` | fixed-capacity limit reached |
| `not_found` | key or section does not exist |
| `invalid_argument` | invalid key/value/section or malformed input |
| `out_of_range` | typed numeric conversion overflow |

## Complexity and determinism

The document uses a fixed-capacity Castle vector and linear key lookup. With
`N = document::size()`, lookup/update/removal are `O(N)` and require no heap
allocation.

Serialization uses a bounded `O(N^2)` section-grouping scan. That trade-off is
intentional: it avoids introducing another associative container only for
serialization. The resulting ordering is deterministic for a given document.

Parsing is a single pass over the input and uses only fixed-capacity temporary
storage for the current section and current decoded value.

## Embedded usage guidance

The INI layer does not perform file or stream I/O. The application owns the
transport/storage boundary and passes a `castle::container::string_view` over
a buffer obtained from flash, EEPROM, a filesystem, a communication frame,
or another source.

This separation keeps the serialization layer platform-independent and avoids
pulling filesystem, stream, allocation, or exception machinery into firmware.

Choose the document capacities from measured configuration bounds. For example:

```cpp
using boot_config = castle::serialization::ini::document<
    24U,  // maximum entries
    24U,  // maximum section name length
    32U,  // maximum key length
    96U   // maximum decoded value length
>;
```

The memory footprint is deterministic because all entry storage is part of the
document object.

## Compatibility notes

The implementation intentionally follows the practical syntax of the earlier
`ini_parser` project: sections, `#`/`;` comments, `=`/`:` assignments,
whitespace trimming, quoted strings, and common boolean spellings.

The Castle version does not carry over STL containers, dynamic allocation,
virtual interfaces, file-stream ownership, or exception-based parse errors.
Those responsibilities are replaced with fixed-capacity storage and
`castle::status` + `error_code` results.
