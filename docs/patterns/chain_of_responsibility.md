# Chain of Responsibility

## Overview
An ordered list of handlers where every handler has its own execution condition (predicate) and behavior (action). A request walks the list in order:

| Predicate | Handler kind | Result for this handler |
| --- | --- | --- |
| true | any | Action runs. `void` action or `handler_flow::continue_chain` moves to the next handler; `handler_flow::stop_chain` ends the chain (`stopped`). |
| false | `make_handler` / `make_default_handler` (`reject_policy::stop`, the default) | Chain ends (`rejected`); no later handler is evaluated. The predicate is a guard. |
| false | `make_skippable_handler` (`reject_policy::skip`) | Handler is skipped, next handler is tried. Combined with `stop_chain` this is classic "first matching handler" dispatch. |

If nothing ends the chain, the result is `completed`. The user manages only the chain object.

Two flavors share the same handler semantics, with no base class, no virtual functions, no RTTI, and no dynamic allocation:

| Flavor | Handlers fixed by | Storage | Use when |
| --- | --- | --- | --- |
| `chain_of_responsibility<Handlers...>` | The chain's type (compile-time pack) | `castle::tuple`, no type erasure | The handler list is known at compile time. Every predicate and action can be inlined. |
| `bounded_chain_of_responsibility<N, void(Args...)>` | A capacity `N`; handlers are registered at runtime | `castle::dsa::array` of `N` `castle::callbacks::function` slots, one per handler | Handlers are registered or cleared at runtime, or have unrelated callable types behind one signature. |

`castle::sigslot::signal` and `castle::events::event_dispatcher` broadcast to every slot and cannot short-circuit, so they are not used as the chain engine. A chain is itself a callable (`operator()`), so it connects to a signal slot as a ready-made "first matching behavior" reaction.

## Header
`#include "castle/patterns/chain_of_responsibility.hpp"`

## Dependencies
- [`../callbacks/function.md`](../callbacks/function.md)
- [`../dsa/array.md`](../dsa/array.md)
- [`../error/status.md`](../error/status.md)
- [`../utility/tuple.md`](../utility/tuple.md)
- `castle/utility/integral_sequence.hpp`, `forward.hpp`, `move.hpp`
- `castle/core/compiler.hpp`, `config.hpp`, `traits.hpp`, `types.hpp`

## Public API
| API | Description |
| --- | --- |
| `handler_flow` | Action result: `continue_chain` (try the next handler; same as a `void` action) or `stop_chain`. |
| `reject_policy` | What a handler does when its predicate is false: `stop` (default) or `skip`. |
| `chain_result` | Result of `process()`: `completed` (nothing ended the chain), `stopped` (an action returned `stop_chain`) or `rejected` (a predicate was false on a `stop` handler). |
| `always_true` | Predicate that accepts any arguments; basis of default handlers. |
| `conditional_handler<Predicate, Action, OnReject>` | Predicate and action stored by value. `OnReject` defaults to `reject_policy::stop`. Predicate: `bool(Args&...)`. Action: `void(Args&...)` (continues) or `handler_flow(Args&...)`. Exposes `process(args...)`, `predicate()`, `action()`, `on_reject`. |
| `make_handler(predicate, action)` | Deduces a handler that ends the chain (`rejected`) when the predicate is false. |
| `make_skippable_handler(predicate, action)` | Deduces a handler that is skipped when the predicate is false. |
| `make_default_handler(action)` | Handler with the `always_true` predicate; it never rejects. |
| `chain_of_responsibility<Handlers...>` | Owns the handlers by value. At least one handler is required. |
| `make_chain(handlers...)` | Deduces a `chain_of_responsibility`. |
| `chain.process(args...)` | Runs handlers in declaration order until one ends the chain. Returns `chain_result`. Arguments reach every handler as lvalues. |
| `chain(args...)` | Same as `process()` but returns `handler_flow` (`continue_chain` for `completed`, `stop_chain` for `stopped` and `rejected`), so a chain can be an action of another chain or a signal slot. |
| `chain.handler_at<I>()` | Compile-time indexed access to a handler. |
| `chain_of_responsibility::capacity()` / `handler_count` | Compile-time number of handlers. |
| `bounded_chain_of_responsibility<N, void(Args...), StorageSize, StorageAlignment>` | Runtime-registered chain with `N` slots. Each slot stores a whole `conditional_handler`, so handler semantics match the static chain. |
| `bounded.add_handler(predicate, action)` | Appends a handler that rejects (ends the chain) on a false predicate. Returns `status::ok`, `full`, `invalid_config` (while processing) or `invalid_callback` (null function pointer or empty `castle::callbacks::function`). |
| `bounded.add_skip_handler(predicate, action)` | Appends a handler that is skipped on a false predicate. Same results as `add_handler()`. |
| `bounded.add_default_handler(action)` | Appends a handler with the `always_true` predicate. |
| `bounded.process(args...)` / `bounded(args...)` | Same semantics as the static chain. `Args` are the signature's parameter types. |
| `bounded.clear()` | Removes all handlers; `invalid_config` while processing. |
| `bounded.size()`, `empty()`, `is_processing()`, `capacity()` | State queries. |

Complexity of `process()` is O(H), at most one predicate call and one action call per handler, with H = `sizeof...(Handlers)` or `N`.

## Usage Example
```cpp
#include "castle/patterns/chain_of_responsibility.hpp"

namespace dp = castle::patterns;

struct request { unsigned code; };
struct stats { unsigned seen = 0U; unsigned handled = 0U; unsigned ignored = 0U; };

auto chain = dp::make_chain(
    // Runs for every request and never rejects; the void action lets the chain continue.
    dp::make_default_handler([](request const&, stats& s) noexcept { ++s.seen; }),
    // Dispatch: skipped when the code differs, ends the chain when it matches.
    dp::make_skippable_handler(
        [](request const& r, stats&) noexcept { return r.code == 7U; },
        [](request const&, stats& s) noexcept
        {
            ++s.handled;
            return dp::handler_flow::stop_chain;
        }),
    dp::make_default_handler([](request const&, stats& s) noexcept { ++s.ignored; }));

request req{7U};
stats counters;
chain.process(req, counters); // seen == 1, handled == 1, ignored == 0 -> chain_result::stopped

// Guard: make_handler() rejects (ends the chain) when the predicate is false.
auto guarded = dp::make_chain(
    dp::make_handler([](request const& r, stats&) noexcept { return r.code != 0U; },
                     [](request const&, stats& s) noexcept { ++s.handled; }),
    dp::make_default_handler([](request const&, stats& s) noexcept { ++s.seen; }));
request bad{0U};
guarded.process(bad, counters); // chain_result::rejected, seen unchanged

// Runtime-registered, compile-time bounded:
dp::bounded_chain_of_responsibility<4U, void(request const&, stats&)> bounded;
bounded.add_default_handler([](request const&, stats& s) noexcept { ++s.ignored; });
```
See [`samples/patterns/chain_of_responsibility.cpp`](../../samples/patterns/chain_of_responsibility.cpp), which also connects a chain to a `castle::sigslot::signal`.

## Constraints & Notes
- No heap allocation, exceptions, RTTI, or virtual functions. Handler count (static) or capacity (bounded) is a compile-time constant.
- A `void` action continues the chain. Return `handler_flow::stop_chain` to end it.
- A false predicate ends the chain by default (`rejected`). Use `make_skippable_handler()` / `add_skip_handler()` when a false predicate should only skip that handler.
- A nested chain used as an action reports `stop_chain` for both `stopped` and `rejected`, so a rejection inside it also ends the outer chain.
- `process()` passes the same lvalue arguments to every handler. Use reference parameters for shared requests and contexts. Rvalue-reference parameters are rejected by `bounded_chain_of_responsibility`.
- The bounded chain stores each handler (predicate and action together) in one inline `function` slot (`StorageSize`, default `castle::inplace_storage_reserved`). Callables must be copy-constructible and together fit that storage. With the defaults on a 64-bit host, a 4-handler chain is 400 bytes. A bounded chain is much larger than one slot, so reference it from a lambda instead of copying it into another chain.
- `add_handler()` and `clear()` fail with `invalid_config` while a `process()` call is active; recursive `process()` from a handler is allowed, and its depth is the caller's responsibility.
- Not thread-safe; use one chain from one execution context or protect it externally.
