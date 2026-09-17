# basic_string

## Overview
`castle::container::basic_string` is a fixed-capacity owning string with embedded storage and an always-maintained null terminator. Use it when text must fit inside a deterministic compile-time buffer instead of a heap-allocated string.

## Header
`#include "castle/container/string.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/types.md`](../core/types.md)
- [`../error/status.md`](../error/status.md)
- [`string_view.md`](string_view.md)

## Public API
| API | Description |
|---|---|
| `basic_string<CharT, N>` | Owning string with space for at most `N` characters plus a trailing terminator. |
| `string<N>`, `u8string<N>` | `char`-based aliases. |
| Constructors / assignment | Default, C-string, string-view, and copy construction/assignment. |
| `size()`, `length()`, `capacity()`, `available()`, `empty()`, `full()` | O(1) size/capacity queries. |
| `begin()/end()`, `cbegin()/cend()`, `data()`, `c_str()` | Direct buffer and iterator access. |
| `operator[]`, `front()`, `back()` | O(1) unchecked character access. |
| `assign()` | Replace contents from a C string or string view; returns `status::invalid_argument` for null C-string input and `status::full` on overflow. |
| `push_back()` / `pop_back()` | O(1) append/remove one character. |
| `append()` | Append a C string, string view, or another `basic_string`; returns `status::full` on overflow. |
| `insert(position, view)` | O(size) insertion with element shifting; returns `status::out_of_range` or `status::full` on failure. |
| `erase(position, count)` | O(size - position) erasure; returns `status::out_of_range` for an invalid start index. |
| `resize(count, value)` | Grow or shrink the string; returns `status::full` when `count > capacity()`. |
| `find()` / `compare()` / `view()` | Search, lexicographic compare, and non-owning view helpers. |
| Comparison operators | Equality and ordering against other `basic_string` instances and `basic_string_view`. |

## Usage Example
See [`samples/sample_string.cpp`](../../samples/sample_string.cpp).

```cpp
castle::container::string<16U> text("AB");
text.append("CD");
text.insert(2U, castle::container::string_view("XY"));
CASTLE_SAMPLE_CHECK(text == castle::container::string_view("ABXYCD"));
```

## Constraints & Notes
- No dynamic allocation; storage for `N + 1` characters is embedded in the object.
- Successful mutating operations always preserve null termination at `data()[size()]`.
- Most mutating operations are O(length) because characters may be copied or shifted.
- `operator[]`, `front()`, and `back()` are unchecked.
- Overflow reports `status::full`; popping an empty string reports `status::empty`.
