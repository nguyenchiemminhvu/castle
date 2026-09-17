# stack_buffer

## Overview
Fixed-capacity LIFO buffer with inline storage. Use it for deterministic stack-like behavior where overflow must be handled explicitly rather than by allocation.

## Header
`#include "castle/container/stack.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [traits](../core/traits.md)
- [types](../core/types.md)
- [error_handler](../core/error_handler.md)
- [array](array.md)
- [initializer_list](initializer_list.md)
- [move](../utility/move.md)

## Public API
| API | Description |
| --- | --- |
| `stack_buffer<T, N>` | LIFO buffer with compile-time capacity `N`. Requires `T` to be trivially copyable and trivially destructible; `N > 0`. |
| Constructors | Default constructor and initializer-list constructor. The initializer-list constructor asserts `list.size() <= N` and pushes in list order. |
| `capacity()`, `size()`, `available()`, `empty()`, `full()` | Capacity/state queries. O(1). |
| `clear()` | Removes all elements. O(1). |
| `push(value)` | Pushes when space is available. Returns `true` on success, `false` when full. O(1). |
| `force_push(value)` | Pushes and replaces the current top element when full. Returns `true` when replacement occurred. O(1). |
| `pop(out)`, `pop()` | Removes the current top element with or without copying it out. Returns `false` when empty. O(1). |
| `peek(index, out)` | Copies the element at logical position `index`, where `0` is oldest and `size()-1` is newest. Returns `false` when out of range. O(1). |
| `top()` | Returns the newest element. O(1); stack must be non-empty. |
| `push_bulk(src, max)` | Pushes up to `max` values. Returns count written. O(min(max, available())). |
| `pop_bulk(dst, max)` | Pops up to `max` values in LIFO order. Returns count read. O(min(max, size())). |

## Usage Example
See `samples/sample_stack.cpp`.

```cpp
castle::container::stack_buffer<uint8_t, 4U> stack{1U, 2U};
stack.push(3U);
uint8_t top = stack.top();
```

## Constraints & Notes
- No dynamic allocation; storage is an internal `array<T, N>`.
- `force_push()` replaces the current newest element when the stack is already full.
- The container has no iterators; references from `top()` remain valid only until a mutating operation changes the top slot.
- Not thread-safe; concurrent access requires external synchronization.
