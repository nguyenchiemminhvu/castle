# Move semantics helper

## Overview

Castle equivalent of `std::move`, built on Castle type traits so no STL header is needed.

## Header

```cpp
#include "castle/utility/move.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `move(x)` | Cast `x` to an rvalue reference so move constructors/assignments can be selected. |

## Example

```cpp
Widget b = castle::move(a);
```

## Embedded notes

- `move()` does not move bytes by itself; it only changes overload selection.
