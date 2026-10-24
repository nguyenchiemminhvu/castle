# Network Callback Dispatcher

## Overview
`castle::net::detail::callback_dispatcher<Callback>` runs a stored `castle::callbacks::function` in place while letting that callback replace or clear itself safely. Invoking in place keeps the state of a `mutable` callable between calls, but destroying or overwriting a callable while it runs would free its captures mid-call. The dispatcher remembers which callback is running and defers any `replace()` or `clear()` aimed at it until it returns.

This is an implementation detail shared by `basic_client` and `basic_server`; applications use it through their `set_*_callback()` and `clear_*_callback()` methods. An owner needs one dispatcher however many callbacks it stores, because only one runs at a time.

## Header
`#include "castle_ext/net/detail/callback_dispatcher.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/utility/forward.hpp`](../../../../include/castle/utility/forward.hpp)
- [`castle/utility/move.hpp`](../../../../include/castle/utility/move.hpp)

## Public API
Template parameter `Callback` is a `castle::callbacks::function` specialization. The class is not copyable.

| API | Description |
|---|---|
| `is_dispatching() const` | True while any callback is executing; owners use it to reject re-entrant processing. |
| `is_running(target) const` | True while `target` is the executing callback. |
| `replace(target, next)` | Moves `next` into `target`, or after the call returns when `target` is executing. |
| `clear(target)` | Empties `target`, or after the call returns when `target` is executing. |
| `invoke(target, args...)` | Calls `target` in place, then applies the replace or clear it requested for itself. The last request made during the call wins. |

## Constraints & Notes
- Holds one pending `Callback`, so it costs one extra callback's inline storage per owner (not per slot).
- Invoking must not be nested; check `is_dispatching()` first.
- No allocation, exceptions, RTTI, or virtual dispatch.
