# tags

## Overview
The iterator tag types define Castle's iterator category hierarchy. They exist so generic code can dispatch on iterator capabilities without depending on the standard library tag types.

## Header
`#include "castle/iterator/tags.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)

## Public API
| API | Description |
| --- | --- |
| `struct input_iterator_tag` | Base tag for readable single-pass iterators. |
| `struct output_iterator_tag` | Tag for write-only output iterators. |
| `struct forward_iterator_tag : input_iterator_tag` | Tag for multipass forward iterators. |
| `struct bidirectional_iterator_tag : forward_iterator_tag` | Tag for iterators that can move forward and backward. |
| `struct random_access_iterator_tag : bidirectional_iterator_tag` | Tag for iterators that also support random-access arithmetic. |

## Usage Example
See `samples/sample_tags.cpp` for a complete standalone example.

```cpp
#include "castle/iterator/tags.hpp"

bool accepts_input(castle::input_iterator_tag);
bool readable = accepts_input(castle::random_access_iterator_tag{});
```

## Constraints & Notes
- These are empty marker types only; they do not store state or implement iterator behavior.
- The inheritance chain is how Castle algorithms select stronger capability overloads.
- Custom iterators should expose one of these tags through `iterator_category`.
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL iterator utilities are involved.
