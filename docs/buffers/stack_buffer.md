# `castle/buffers/stack_buffer.h` — Bounded LIFO stack

**Header:** `castle/buffers/stack_buffer.h`
**Namespace:** `castle::buffers`
**Sample:** [`samples/sample_stack_buffer.cpp`](../../samples/sample_stack_buffer.cpp)

## Purpose

`stack_buffer<T, N>` is a fixed-capacity, heap-free **LIFO** stack. Same
guarantees and constraints as `ring_buffer`, but with push/pop
happening at the top instead of the FIFO endpoints. Useful for
recursion-avoidance patterns, expression evaluation, back-tracking
algorithms, and small state save/restore stacks on ISRs.

## Design notes

- Compile-time `N`, inline storage, no heap, no exceptions.
- Requires `T` to be trivially copyable and trivially destructible.
- All observers / mutators are `noexcept`.
- Non-copyable / non-movable.
- Iterators walk oldest → newest (`*(end() - 1)` is the top of the
  stack). Random-access category, `reverse_iterator` supported.

## API (excerpt)

| Method                          | Notes                              |
| ------------------------------- | ---------------------------------- |
| `capacity()`                    | `constexpr` = `N`                  |
| `size()`, `empty()`, `full()`   | O(1)                               |
| `available()`                   | `N - size()`                       |
| `clear()`                       | O(1)                               |
| `push(v)`                       | `false` when full                  |
| `pop(T& out)` / `pop()`         | `false` when empty                 |
| `top()`                         | UB when empty (precondition)       |
| `begin/end`, `rbegin/rend`      | Random-access iteration            |

## Example

```cpp
#include <castle/buffers/stack_buffer.h>
using namespace castle::buffers;

stack_buffer<int, 16> undo;
undo.push(current_state_id);

int prev;
if (undo.pop(prev)) restore(prev);
```

## See also

- `castle/buffers/ring_buffer.h` — FIFO counterpart.
