# Integral sequences

## Overview

Compile-time sequences of integral constants, similar to `std::integer_sequence`, for generic template code.

## Header

```cpp
#include "castle/utility/integral_sequence.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `integer_sequence<T, ...>` | Represents a compile-time list of integral values. |
| `index_sequence` | Convenience sequence of `size_t` indexes. |
| Sequence builders | Generate `index_sequence` values for tuples and parameter packs. |

## Example

```cpp
template <size_t... I> void use(castle::index_sequence<I...>) { /* ... */ }
use(castle::make_index_sequence<4>{});
```

## Embedded notes

- This header is useful when expanding tuples or forwarding parameter packs without STL.
