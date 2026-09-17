# Perfect forwarding

## Overview

Castle equivalents of the standard perfect-forwarding primitives.

## Header

```cpp
#include "castle/utility/forward.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `forward<T>(x)` | Preserve lvalue/rvalue category when passing a value to another template. |
| Reference collapsing support | Built on Castle type traits instead of STL `<utility>`. |

## Example

```cpp
template <typename T> void make(T&& x) { target(castle::forward<T>(x)); }
```

## Embedded notes

- Use this in forwarding constructors and `emplace` functions.
