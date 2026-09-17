# string_view

## Overview
Non-owning view over a character sequence. Use it to inspect, compare, and slice text without copying and without requiring the viewed range to own or allocate storage.

## Header
`#include "castle/container/string_view.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [types](../core/types.md)

## Public API
| API | Description |
| --- | --- |
| `basic_string_view<CharT>` | Pointer/length view over contiguous characters. |
| `string_view` | Alias for `basic_string_view<char>`. |
| Constructors | Default constructor, pointer/length constructor, and null-terminated string constructor. The null-terminated constructor is O(N); others are O(1). |
| `size()`, `length()`, `empty()`, `data()` | View state queries. O(1). |
| `operator[]`, `front()`, `back()` | Character access. O(1); caller must respect bounds/non-empty preconditions. |
| `begin()`, `end()` | Pointer iterators over the viewed range. O(1). |
| `substr(offset, count)` | Returns a clamped child view or an empty default view when `offset > size()`. O(1). |
| `find(CharT, offset)` | Finds a character and returns its index or `npos`. O(N) worst-case. |
| `find(string_view, offset)` | Finds a substring and returns its index or `npos`. O(N*M) worst-case. |
| `compare(other)` | Lexicographical comparison. Returns negative/zero/positive. O(min(N, M)). |
| `starts_with(prefix)`, `ends_with(suffix)` | Prefix/suffix tests. O(prefix.size()) / O(suffix.size()). |
| `npos` | Sentinel returned when searches fail. |
| `==`, `!=`, `<`, `>`, `<=`, `>=` | Free lexicographical comparison operators built on `compare()`. |

## Usage Example
See `samples/sample_string_view.cpp`.

```cpp
castle::container::string_view text("SET:TEMP=25");
auto key = text.substr(4U, 4U);
bool ok = text.starts_with(castle::container::string_view("SET:"));
```

## Constraints & Notes
- No allocation; the view stores only a pointer and a length.
- The referenced characters must outlive the view.
- Iterator/reference invalidation follows the underlying storage: destroying or relocating the source buffer invalidates the view.
- Not thread-safe; coordinate concurrent mutation of the referenced storage externally.
