# Decorator

## Overview
Wraps a callable, such as a `castle::callbacks::function`, in outer layers that add behavior (tracing, argument checks, clamping, retry, caching, guards) without changing how it is called. It uses templates instead of the classic inheritance hierarchy.

A *decorator* is any callable invocable as `decorator(next, args...)`. It receives the wrapped callable as `next` and decides what happens around the call:

| Decorator body | Effect |
| --- | --- |
| code, then `return next(args...)` | Pre-processing (trace, count, validate) |
| `auto r = next(args...)`, code, `return r` | Post-processing (clamp, convert, record) |
| `return next(changed_args...)` | Argument rewriting |
| return without calling `next` | Guard / short-circuit / cache hit |
| call `next` in a loop | Retry |

This is the Python decorator model. A Python decorator returns a wrapper that closes over `func`; here the wrapper body is the decorator itself and `func` is passed in as `next`:

```text
# Python                      // CASTLE
@trace                        auto f = castle::patterns::decorate(target, trace, retry);
@retry                        f(args...);   // trace -> retry -> target
def target(...): ...
```

C++ has no `@decorator` syntax at a declaration, and rebinding a function name at runtime would mean global hook state. CASTLE builds a new callable value instead: `decorate()` returns a `decorated` object that owns the target and its decorators by value.

GoF mapping:

| GoF role | CASTLE |
| --- | --- |
| Component interface | The call signature (static duck typing, no base class) |
| ConcreteComponent | The target: function pointer, lambda, functor, `callbacks::function`, delegate, execution policy |
| Decorator (holds and forwards to the wrappee) | `decorated<Target, Decorator>` |
| ConcreteDecorator | Each decorator callable |

## Header
`#include "castle/patterns/decorator.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/traits.md`](../core/traits.md)
- [`../utility/forward.md`](../utility/forward.md)

Works with, but does not include, [`../callbacks/function.md`](../callbacks/function.md) and [`../callbacks/exec_policy.md`](../callbacks/exec_policy.md).

## Public API
| API | Description |
| --- | --- |
| `decorated<Target, Decorator>` | One decorator layer around one target. Both are stored by value. |
| `decorated(target, decorator)` | Constructs the layer by forwarding (copying or moving) both callables. `constexpr`. |
| `decorated::operator()(args...)` | Calls `decorator(target, args...)`, forwarding `args` unchanged, and returns the decorator's result unchanged (references included). Fails to compile with a clear message if the decorator cannot be called that way. `constexpr`, non-`const`. |
| `decorated::target()` | Reference to the wrapped callable, e.g. to rebind a wrapped `callbacks::function` or to reach an inner layer. |
| `decorated::decorator()` | Reference to the decorator, e.g. to read a functor decorator's counters. |
| `decorated::target_type`, `decorator_type` | The stored types. |
| `decorate(target, decorator)` | Deduces `decorated<decay_t<Target>, decay_t<Decorator>>`. A function name decays to a function pointer. |
| `decorate(target, d1, d2, ..., dn)` | Stacks decorators. `d1` is the outermost layer and runs first; `dn` calls the target. Same type as `decorate(decorate(target, d2, ..., dn), d1)`. |

Each layer costs one call, and the compiler can inline it. There is no type erasure unless you request it.

## Usage Example
```cpp
#include "castle/patterns/decorator.hpp"
#include "castle/callbacks/function.hpp"

namespace dp = castle::patterns;
namespace cb = castle::callbacks;

cb::function<int32_t(uint8_t)> raw_read = [&adc](uint8_t channel) noexcept { return adc.read(channel); };

auto retry = [](auto& next, uint8_t channel) noexcept
{
    int32_t result = -1;
    for (uint32_t attempt = 0U; attempt < 3U && result < 0; ++attempt)
    {
        result = next(channel);
    }
    return result;
};

auto clamp_range = [](auto& next, uint8_t channel) noexcept
{
    int32_t const value = next(channel);
    return value > 3300 ? int32_t{3300} : value;
};

// clamp_range -> retry -> raw_read
auto safe_read = dp::decorate(raw_read, clamp_range, retry);
int32_t const mv = safe_read(2U);

// Optional type erasure: store the whole stack in a function sized for it.
cb::function<int32_t(uint8_t), sizeof(safe_read)> erased = safe_read;

// A generic decorator can be reused for any signature.
auto count_calls = [&calls](auto& next, auto&&... args) -> decltype(auto)
{
    ++calls;
    return next(castle::forward<decltype(args)>(args)...);
};
```
See [`samples/patterns/decorator.cpp`](../../samples/patterns/decorator.cpp): a `castle::callbacks::function` is wrapped with a call-counting decorator (before the call) and a result-limiting decorator (after the call).

## Constraints & Notes
- No heap allocation, exceptions, RTTI, virtual functions, macros, or global/static state. Every `decorated` object is independent. Copying one copies the target and the decorators, including their state.
- Size is the sum of the target and decorators, plus padding. Wrapping a `callbacks::function` embeds a full copy of it, so a decorated function does not fit back into a function of the same `StorageSize`. To erase it, use a `callbacks::function<Sig, sizeof(decorated_object)>` or a larger one. To avoid the copy, pass `castle::move(fn)`, or use a lambda that captures the function by reference as the target. That target must outlive the decorated object.
- `decorated::operator()` is non-`const` so decorators and targets may keep state (counters, caches, mutable lambdas). `callbacks::function` can still store and invoke it.
- Arguments reach the decorator exactly as the caller passed them (perfect forwarding). A decorator that calls `next` several times (retry) must not move from an argument before the last call. Taking parameters by value or by lvalue reference avoids this.
- Write decorators as lambdas with `auto& next` or as functors with a templated call operator. One decorator then works for any target. A plain function cannot be a decorator because it cannot name the type of `next`.
- A wrapped empty `callbacks::function` asserts when a decorator calls it. A decorator can test it first with `if (next)`.
- Relation to [`exec_policy`](../callbacks/exec_policy.md): `once`, `every_n`, `throttle`, etc. are ready-made gating wrappers for callbacks. They compose with `decorate()` in both directions: as a target (`decorate(make_every_n(2U, cb), trace)`) or around a decorated callable (`make_once(decorate(cb, trace))`).
- Not thread-safe by itself. Synchronize decorators that share state, or call each decorated object from one execution context.
