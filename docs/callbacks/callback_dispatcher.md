# Callback Dispatcher

## Overview
`callback_dispatcher.hpp` provides `castle::callbacks::callback_dispatcher<Callback>`, which runs a stored `castle::callbacks::function` in place while letting that callback replace or clear itself safely. Invoking in place keeps the state of a `mutable` callable between calls, but destroying or overwriting a callable while it runs would free its captures mid-call. The dispatcher remembers which callback is running and defers any `replace()` or `clear()` aimed at it until it returns.

This lets an owner accept a new callback from inside the currently executing one (lock-free, no allocation). The network modules `castle_ext/net` (`basic_client`, `basic_server`) and the DNS client use it through their `set_*_callback()` and `clear_*_callback()` methods. An owner needs one dispatcher however many callbacks it stores, because only one runs at a time.

## Header
`#include "castle/callbacks/callback_dispatcher.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../include/castle/core/compiler.hpp)
- [`castle/utility/forward.hpp`](../../include/castle/utility/forward.hpp)
- [`castle/utility/move.hpp`](../../include/castle/utility/move.hpp)

## Public API
Template parameter `Callback` is a `castle::callbacks::function` specialization. The class is not copyable.

| API | Description |
|---|---|
| `is_dispatching() const` | True while any callback is executing; owners use it to reject re-entrant processing. |
| `is_running(target) const` | True while `target` is the executing callback. |
| `replace(target, next)` | Moves `next` into `target`, or after the call returns when `target` is executing. |
| `clear(target)` | Empties `target`, or after the call returns when `target` is executing. |
| `invoke(target, args...)` | Calls `target` in place, then applies the replace or clear it requested for itself. The last request made during the call wins. |

## Usage Example
See `samples/callbacks/callback_dispatcher.cpp`.

```cpp
using callback_type = castle::callbacks::function<void(uint32_t), 64U, 8U>;

castle::callbacks::callback_dispatcher<callback_type> dispatcher;
callback_type callback{};
uint32_t total = 0U;

dispatcher.replace(callback, callback_type{[&](uint32_t v) noexcept
{
    total += v;
    dispatcher.replace(callback, callback_type{[&](uint32_t) noexcept {}}); // applied after this call returns
}});
dispatcher.invoke(callback, 3U); // total == 3, callback now the replacement
```

## Constraints & Notes
- Holds one pending `Callback`, so it costs one extra callback's inline storage per owner (not per slot).
- Invoking must not be nested; check `is_dispatching()` first.
- No allocation, exceptions, RTTI, or virtual dispatch.
