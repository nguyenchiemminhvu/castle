# Forward

## Overview
`castle::forward` preserves the value category of a function argument. It exists so Castle templates can implement perfect forwarding without depending on `<utility>`.

## Header
`#include "castle/utility/forward.hpp"`

## Dependencies
[compiler.md](../core/compiler.md), [traits.md](../core/traits.md)

## Public API
| API | Description |
|---|---|
| `forward<T>(remove_reference_t<T>& value)` | Forwards an lvalue as `T&&`. |
| `forward<T>(remove_reference_t<T>&& value)` | Forwards an rvalue as `T&&`; rejects forwarding an rvalue as an lvalue. |

## Usage Example
See `samples/sample_forward.cpp`.

```cpp
template <typename T>
void relay(T&& value)
{
    sink(castle::forward<T>(value));
}
```

## Constraints & Notes
- Pure cast helper; it does not move or copy data by itself.
- No heap allocation, exceptions, RTTI, or virtual dispatch.
- The rvalue overload contains a static assertion that prevents misuse.
