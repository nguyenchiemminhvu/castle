# Composite

## Overview

`castle::patterns::composite` groups a fixed set of heterogeneous objects into a tree and lets clients apply a generic operation to the group as a whole. It uses compile-time composition and a caller-supplied visitor instead of a shared base class, virtual functions, or domain-specific methods. Nested groups are traversed recursively, so callers can operate on all leaf objects without manually walking each cluster.

Traversals can run in declaration order or mirrored (reverse) order, and visitors may report failures with `castle::status`. This makes the composite suitable for fan-out operations such as start/stop, suspend/resume, or reset across clusters of services: a component that lacks the operation is a compile error instead of a silently forgotten call, startup can fail fast, and teardown can run in reverse and still visit every component.

A composite may own components by value or hold non-owning references. This supports both locally managed objects and services whose lifetime is managed by a containing application or service.

## Header

`#include "castle/patterns/composite.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../include/castle/core/compiler.hpp)
- [`castle/core/traits.hpp`](../../../include/castle/core/traits.hpp)
- [`castle/core/types.hpp`](../../../include/castle/core/types.hpp)
- [`castle/error/status.hpp`](../../../include/castle/error/status.hpp)
- [`castle/utility/forward.hpp`](../../../include/castle/utility/forward.hpp)
- [`castle/utility/integral_sequence.hpp`](../../../include/castle/utility/integral_sequence.hpp)
- [`castle/utility/tuple.hpp`](../../../include/castle/utility/tuple.hpp)

The dependencies above are links to the corresponding public headers because the supplied repository snapshot does not contain documentation files for these foundational headers.

## Public API

| API | Description |
|---|---|
| `composite<Components...>` | Fixed heterogeneous composition. Value template arguments are owned; lvalue-reference template arguments are non-owning. The default constructor is available when every value child is default-constructible; the variadic constructor forwards one initializer per child. |
| `make_composite(components...)` | Creates a composite that stores decayed component values, copying lvalues and moving rvalues. |
| `tie_composite(components...)` | Creates a composite of lvalue references without copying the objects. |
| `forward_order` / `reverse_order` | Order tags accepted as an optional first traversal argument. `forward_order` (default) is depth-first, left-to-right; `reverse_order` is the exact mirror of the forward sequence. |
| `is_composite<T>` | Trait reporting whether `T` is a `composite`, ignoring cv- and reference qualifiers. Useful in `for_each_child()` / `for_each_node()` visitors. |
| `size()` | Returns the compile-time number of immediate children. |
| `leaf_size()` | Returns the compile-time number of leaf objects, recursively excluding composite nodes. |
| `empty()` | Reports whether the composite has immediate children. |
| `get<Index>()` | Returns an immediate child by compile-time index. |
| `for_each([order,] visitor)` | Applies a visitor to every leaf; nested composite nodes are expanded. Visits all leaves and returns the first failure, or `status::ok`. |
| `try_for_each([order,] visitor)` | Like `for_each()`, but stops at the first failing leaf (including leaves of enclosing groups) and returns that status. `[[nodiscard]]`. |
| `for_each_child([order,] visitor)` | Applies a visitor to immediate children only; nested groups are passed as group objects. Returns the first failure. |
| `for_each_node([order,] visitor)` | Visits every descendant, including nested groups; the root is not passed. Forward order is pre-order (group before descendants); reverse order mirrors it (descendants before their group). Returns the first failure. |

Traversal visitors are invoked as lvalues and must be callable with every object type they receive. They must return `void` (treated as success) or `castle::status`; any other return type is rejected at compile time so error codes cannot be silently dropped. A generic visitor may return `void` for some types and `castle::status` for others. `for_each()` requires an operation valid for every leaf type. `for_each_child()` and `for_each_node()` must also accept composite node types when present.

## Usage Example

```cpp
#include "castle/error/status.hpp"
#include "castle/patterns/composite.hpp"

namespace dp = castle::patterns;

struct motor
{
    castle::status start() noexcept { return castle::status::ok; }
    castle::status stop() noexcept { return castle::status::ok; }
};

struct valve
{
    castle::status start() noexcept { return castle::status::ok; }
    castle::status stop() noexcept { return castle::status::ok; }
};

struct logger
{
    void start() noexcept {}
    void stop() noexcept {}
};

int main()
{
    motor pump;
    valve outlet;
    logger diagnostics;

    // The application owns every object; groups only refer to them.
    auto hydraulics = dp::tie_composite(pump, outlet);

    // A composite can itself be a child of another composite.
    auto system = dp::make_composite(hydraulics, dp::tie_composite(diagnostics));

    // Fail fast during startup: pump -> outlet -> diagnostics.
    castle::status started = system.try_for_each([](auto& component)
    {
        return component.start();
    });
    if (started != castle::status::ok)
    {
        return 1;
    }

    // Stop every component in reverse order: diagnostics -> outlet -> pump.
    castle::status stopped = system.for_each(dp::reverse_order, [](auto& component)
    {
        return component.stop();
    });

    return (stopped == castle::status::ok) ? 0 : 1;
}
```

See `samples/patterns/composite.cpp` for a minimal standalone example: components from application, service, and HAL layers (including a nested bus-driver group) are wrapped in one system composite that shuts every component down with a single call. Groups may be nested by reference with `tie_composite()`, so no instance is copied.

A subsystem that manages its own internal components can either expose them for inclusion in an outer group, or stay a single leaf whose operation forwards to a private composite member (for example `parts_.for_each(dp::reverse_order, ...)`).

## Constraints & Notes

- **No dynamic allocation:** components are stored in `castle::tuple`; traversal uses compile-time index sequences and fold expressions.
- **No inheritance, virtual functions, or RTTI:** concrete component types are known at compile time and visitors are statically dispatched.
- **Deterministic order:** forward traversal is depth-first and left-to-right, following template argument order; `reverse_order` visits exactly the mirrored sequence. The order is fixed at compile time and costs nothing at runtime.
- **Fixed composition:** child count and child types are compile-time properties. There is deliberately no runtime `add()` or `remove()` API. A runtime heterogeneous registry would need a user-defined erased operation interface; it cannot remain universally generic for arbitrary visitor types. For a runtime list of callbacks sharing one signature, use `castle::callbacks::delegate_registry` or `callback_dispatcher`.
- **Ownership:** `make_composite()` owns decayed values. `tie_composite()` stores references; every referenced object must outlive the composite and any outer composite storing it. A composite holding references is copyable but not assignable.
- **Nested value groups:** an outer composite may own an inner composite by value. An inner reference composite continues to refer to the original external objects when copied or moved.
- **Complexity:** `for_each()` and `for_each_node()` are O(N) in the number of descendants. `for_each_child()` is O(C) in the number of immediate children. Traversal uses no dynamic or container storage; runtime call-stack depth is proportional to the nesting depth.
- **Thread safety:** no synchronization is performed. Concurrent traversal or mutation of referenced child objects requires application-level synchronization.
- **Error handling:** visitor results are `void` or `castle::status`. `for_each()`, `for_each_child()`, and `for_each_node()` always visit every object and return the first failure. `try_for_each()` stops at the first failure; it is provided for leaf traversal only. Per-component logging or recovery belongs in the visitor.
- **CASTLE primitive selection:** this feature uses `castle::meta`, `castle::tuple`, `castle::sequence`, and `castle::status` because heterogeneous child types, compile-time traversal, and exception-free error reporting are central to the design. `castle::dsa::array` and `castle::callbacks::function` are intentionally not used: a homogeneous runtime array or a fixed erased callback signature would constrain the generic visitor API and add per-call indirection without adding value.
