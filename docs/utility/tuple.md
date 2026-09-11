# Tuple

## Overview

A compile-time heterogeneous tuple implemented without the STL. It stores each element by type/index and supports generic access.

## Header

```cpp
#include "castle/utility/tuple.h"
```

**Namespace:** `castle`

## Main API

| API | Purpose |
|---|---|
| `tuple<T...>` | Store a fixed number of values of different types. |
| `get<I>(t)` / typed get | Access an element by index or supported type form. |
| `tuple_size` / `tuple_element` | Compile-time tuple metadata. |
| Construction helpers | Tuple constructors and `make_tuple`-style helpers where provided. |
| Apply/visit helpers | Use tuple contents with a callable where supplied by the header. |

## Example

```cpp
auto t = castle::make_tuple(10, 3.5f);
int a = castle::get<0>(t);
float b = castle::get<1>(t);
```

## Embedded notes

- The implementation is recursive/template-based and stores elements inline.
- Use it for small fixed argument packs; avoid very large tuples on deeply constrained builds.
