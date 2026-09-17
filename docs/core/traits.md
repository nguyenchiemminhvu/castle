# Compile-time type traits

## Overview

A large set of STL-independent type traits and type transformations used throughout Castle. This is the core metaprogramming layer.

## Header

```cpp
#include "castle/core/traits.h"
```

**Namespace:** `castle::meta and castle`

## Main API

| API | Purpose |
|---|---|
| Type predicates | Examples include `is_same`, `is_void`, `is_reference`, `is_array`, `is_object`, `is_integral`, `is_floating_point`, `is_signed`, `is_unsigned`. |
| Construction/destruction traits | `is_constructible`, `is_nothrow_constructible`, `is_copy_constructible`, `is_move_constructible`, `is_destructible`, and related checks. |
| Type transformations | `remove_reference`, `remove_const`, `remove_volatile`, `decay`, `make_unsigned`, `add_pointer`, and related forms. |
| Logic helpers | `enable_if`, conjunction/disjunction/negation-style traits, and `bool_constant`. |
| Utility traits | Traits such as `is_valid_integer` and helpers needed by Castle containers and chrono. |
| `in_place_t` | Tag type used by facilities such as `optional` for direct construction. |

## Example

```cpp
static_assert(castle::meta::is_integral<unsigned>::value, "must be integral");
using raw_t = castle::meta::decay_t<const int&>;
```

## Embedded notes

- This header is intentionally independent of STL `<type_traits>`.
- Prefer the provided traits when writing code that must keep the Castle dependency boundary clean.
