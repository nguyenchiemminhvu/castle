# Delegate

## Overview
`delegate.hpp` provides signature-aware callback wrappers for free functions, functors, lambdas, and member functions. Use it when you want explicit binding semantics, no heap allocation, and predictable storage/lifetime costs.

## Header
`#include "castle/callbacks/delegate.hpp"`

## Dependencies
- [`../core/compiler.md`](../core/compiler.md)
- [`../core/config.md`](../core/config.md)
- [`../core/traits.md`](../core/traits.md)
- [`../utility/forward.md`](../utility/forward.md)
- [`../utility/move.md`](../utility/move.md)
- [`../utility/tuple.md`](../utility/tuple.md)

## Public API
| API | Description |
| --- | --- |
| `delegate_base<R(Args...)>` | Abstract interface used by non-owning registries; exposes `operator()(Args...)`. |
| `delegate_ptr<R(Args...)>` | Stores a runtime free/static function pointer. |
| `delegate_ft<Callable, R(Args...), StorageSize, StorageAlignment>` | Stores a callable by value with compile-time size/alignment checks. |
| `make_delegate_ft<Signature>(callable)` | Deduce the callable type and return `delegate_ft`. |
| `delegate_ftr<Callable, R(Args...)>` | Stores a pointer to an existing callable object. |
| `make_delegate_ftr<Signature>(callable)` | Deduce the callable type and return `delegate_ftr`. |
| `delegate_member<Obj, R(Args...)>` | Stores an object pointer plus a runtime member-function pointer. |
| `delegate_ptr_ct<&func>` | Compile-time-bound free/static function delegate with no runtime function-pointer storage. |
| `delegate_ft_ct<Callable, R(Args...)>` | Compile-time-bound default-constructible functor delegate. |
| `delegate_member_ct<&Obj::member>` | Compile-time-bound member function with a runtime object reference. |
| `delegate_ins_ct<instance, &Obj::member>` | Compile-time-bound member function and compile-time-bound instance reference. |

## Usage Example
See `samples/sample_delegate.cpp`.

```cpp
struct counter
{
    int total = 0;

    void add(int value) noexcept
    {
        total += value;
    }
};

counter value{};
castle::callbacks::delegate_member<counter, void(int)> callback{value, &counter::add};
callback(3);
```

## Constraints & Notes
- No heap allocation.
- No RTTI-based type erasure.
- `delegate_ft` owns its callable by value; `delegate_ftr`, `delegate_member`, and registry users borrow external objects.
- `delegate_registry` stores `delegate_base*`, so the callback object must outlive the active registration.
- No internal synchronization; thread-safety depends on the bound target and external coordination.
