# Core Types

## Overview
This header defines Castle's shared size and difference aliases so generic code can use one consistent vocabulary across targets.

## Header
`#include "castle/core/types.hpp"`

## Dependencies
- [compiler.md](compiler.md)

## Public API
| API | Description |
|---|---|
| `castle::size_type` | Alias for the target's unsigned size/index type. |
| `castle::difference_type` | Alias for the target's signed pointer-difference type. |

## Usage Example
```cpp
#include "castle/core/types.hpp"

castle::size_type count = 8U;
castle::difference_type delta = -1;
```

See also: `samples/sample_types.cpp`.

## Constraints & Notes
- Aliases map directly to the target ABI's fundamental C types.
- The header has no runtime behavior and introduces no storage.
