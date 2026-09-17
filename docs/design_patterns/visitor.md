# Visitor

## Overview
Variadic visitor and visitable base helpers for classic visitor-style dispatch. Use it when concrete node types need a shared visitor interface with compile-time-checked overload sets.

## Header
`#include "castle/design_patterns/visitor.hpp"`

## Dependencies
- `castle/core/traits.hpp`

## Public API
| API | Description |
| --- | --- |
| `visitor<T1, Types...>` | Combines multiple `visit()` overload requirements through recursive inheritance. |
| `visitor<T1>` | Base case declaring one pure-virtual `visit(T1)`. |
| `visitable<T1, Types...>` | Combines multiple `accept()` overload requirements through recursive inheritance. |
| `visitable<T1>` | Base case declaring one pure-virtual `accept(T1&)`. |

## Usage Example
```cpp
// See: samples/sample_visitor.cpp
#include "castle/design_patterns/visitor.hpp"

struct start_command;
struct stop_command;
using command_visitor = castle::design_patterns::visitor<start_command&, stop_command&>;
```

## Constraints & Notes
- No heap allocation or RTTI.
- This implementation uses pure virtual functions for `visit()` and `accept()`, so dispatch is runtime polymorphic rather than CRTP/static dispatch.
- Parameter packs require unique types; duplicates are rejected with `static_assert`.
