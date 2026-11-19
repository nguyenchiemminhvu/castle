# Object Graph

## Overview

`castle::patterns::object_graph` is a compile-time *composition root*. In one type declaration it describes every object of an application or feature context and how the objects relate. It owns those objects, constructs and wires them in the right order, and drives their start-up and shutdown.

It replaces the hand-written `app_context` struct:

```cpp
struct app_context
{
    uart_config config;
    uart_node   uart{config};      // must come after config
    data_pool   pool;
    ubx_parser  ubx{pool};         // must come after pool
    // ... plus init()/deinit() that call every member in order and undo a failed start-up
};
```

Each object names its relations to earlier objects:

| Relation | UML meaning | Constructor receives | Rule |
|---|---|---|---|
| `init_with<T...>` | construction input ("init with") | `const T&` | read-only |
| `uses<T...>` | aggregation | `T&` | any number of users |
| `owns<T...>` | composition | `T&` | at most one owner per object |

Declaration order is lifetime order. An object must be declared after everything it refers to. It is then constructed after those objects and destroyed before them, so no reference can dangle. The compiler rejects a relation that breaks this rule, names a type missing from the graph, or gives a part two owners.

Construction only wires references and cannot fail. Work that can fail goes in optional `init()` / `deinit()` hooks on the objects. `object_graph::init()` runs the hooks in declaration order and rolls back on failure. `object_graph::deinit()` runs them in reverse order.

## Header

`#include "castle/patterns/object_graph.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../core/compiler.md)
- [`castle/core/status.hpp`](../core/status.md)
- [`castle/core/traits.hpp`](../core/traits.md)
- [`castle/core/types.hpp`](../core/types.md)
- [`castle/patterns/composite.hpp`](composite.md)
- [`castle/utility/integral_sequence.hpp`](../utility/integral_sequence.md)
- [`castle/utility/tuple.hpp`](../utility/tuple.md)

## Public API

### Declaring a graph

| API | Description |
|---|---|
| `object_graph<Entries...>` | Owns one object per entry, in lifetime order. An entry is a plain type `T` or `object<T, Relations...>`. Requires at least one entry. Object types must be unique. |
| `object<T, Relations...>` | Describes one object. `T` is constructed with one argument per related object, in the order the relations and their types are written. `object<T>` (or a plain `T`) is value-initialized. `T` must be a non-cv, non-array object type. |
| `init_with<Ts...>` | Read-only construction input: passes each `T` as `const T&`. |
| `uses<Ts...>` | Aggregation: passes each `T` as `T&`. The same object may be used by many objects. |
| `owns<Ts...>` | Composition: passes each `T` as `T&`. An object may be owned by at most one object; other objects may still `uses<>` it. |

Example: `object<hal_adapter, init_with<hal_config>, owns<uart_node, gpio_led>, uses<data_pool>>` requires the constructor `hal_adapter(const hal_config&, uart_node&, gpio_led&, data_pool&)`.

### Members of `object_graph`

| Member | Description |
|---|---|
| `object_graph()` | Constructs every object in declaration order, passing each one its related objects. Objects are destroyed in reverse order. |
| `~object_graph()` | Calls `deinit()` when the graph is still initialized (result discarded), then destroys the objects. |
| copy / move | Deleted: objects hold references into the graph. |
| `static constexpr size_type object_count` / `size()` | Number of objects. |
| `static constexpr bool contains<T>()` | Whether `T` is an object of the graph. |
| `T& get<T>()` / `const T& get<T>() const` | Typed access. Any other type is a compile error. O(1), no runtime lookup. |
| `objects_type objects()` / `const_objects_type objects() const` | A non-owning `composite<T&...>` of every object in declaration order, for custom fan-out (`for_each_child`, `reverse_order`, ...). See [composite](composite.md). |
| `status init()` | Calls each object's `init()` hook in declaration order and stops at the first failure. On failure it calls `deinit()` on the objects already initialized, in reverse order, then returns the failing status; the graph stays uninitialized. Returns `status::already_exists` if already initialized. |
| `status deinit()` | Calls every `deinit()` hook in reverse declaration order, even after a failure, and returns the first failure. Returns `status::not_configured` if not initialized. |
| `bool is_initialized() const` | `true` after a successful `init()` until `deinit()`. |

### Lifecycle hooks

An object may define `init()` and/or `deinit()` that take no arguments and return `void` (always succeeds) or `castle::status`. Any other return type is a compile error. Objects without a hook are skipped, so plain configuration and data objects need nothing. Hooks run on every object, owned parts included. A part is initialized before its owner and deinitialized after it, the same order C++ uses for data members.

### Compile-time diagnostics

| Mistake | Diagnostic |
|---|---|
| Relation to an object declared later | `a related object must be declared before the object that refers to it, so it is constructed first and destroyed last` |
| Relation to a type missing from the graph | `a relation names a type that is not an object of this graph` |
| Two `owns<>` of the same object | `an object can be owned (owns<>) by at most one other object` |
| Same type listed twice | `every object type must appear only once` |
| Constructor does not match the relations | `object must be constructible from its relations, in declaration order ...` |
| Relation argument is not `init_with`/`uses`/`owns` | `object relations must be init_with<>, uses<> or owns<>` |
| Hook returns something else | `init() must return void or castle::status` (same for `deinit()`) |
| `get<T>()` for a type missing from the graph | `type is not an object of this graph` |

## Usage Example

```cpp
#include "castle/patterns/object_graph.hpp"

namespace dp = castle::patterns;

struct uart_config { uint32_t baud = 115200U; };       // values from device tree, Kconfig, ...

struct uart_node
{
    explicit uart_node(const uart_config& config);
    castle::status init();                             // open the device; may fail
    void deinit();
};

struct gpio_led    { void init(); void deinit(); };
struct data_pool   { /* latest position */ };          // no hooks: skipped
struct ubx_parser  { explicit ubx_parser(data_pool& pool); };
struct nmea_parser { explicit nmea_parser(data_pool& pool); };

struct hal_adapter
{
    hal_adapter(uart_node&, gpio_led&, ubx_parser&, nmea_parser&, data_pool&);
    castle::status init();                             // parts are already initialized
    void deinit();                                     // runs before the parts stop
};

using location_context = dp::object_graph<
    uart_config,
    dp::object<uart_node,   dp::init_with<uart_config>>,
    gpio_led,
    data_pool,
    dp::object<ubx_parser,  dp::uses<data_pool>>,
    dp::object<nmea_parser, dp::uses<data_pool>>,
    dp::object<hal_adapter, dp::owns<uart_node, gpio_led, ubx_parser, nmea_parser, data_pool>>>;

location_context context;                              // static storage, wired in one place

int main()
{
    if (context.init() != castle::status::ok)          // uart -> led -> hal, rollback on failure
    {
        return -1;
    }

    context.get<hal_adapter>();                        // typed access
    // ...
    return (context.deinit() == castle::status::ok) ? 0 : -1;  // hal -> led -> uart
}
```

See `samples/castle/patterns/object_graph.cpp` for the full, runnable version that checks the hook order.

To construct a graph at a chosen time (for example after the kernel has started) and still keep it in static storage, wrap it in a [singleton](singleton.md): `castle::patterns::singleton<location_context>::create()` and `destroy()`.

## Constraints & Notes

- **No dynamic allocation:** every object is a direct subobject of the graph. The graph lives wherever it is declared: namespace scope or function `static` (static storage), or a local variable (stack; mind the stack size for large contexts).
- **No exceptions, RTTI, or virtual functions:** wiring is resolved at compile time; `get<T>()` compiles to a fixed offset.
- **Deterministic order:** construction and `init()` follow declaration order; `deinit()` and destruction follow the exact reverse. A graph defined at namespace scope is one object, so its members cannot suffer cross-translation-unit static initialization order problems.
- **Fixed structure:** the set of objects and their relations are compile-time properties. Runtime insertion or removal is intentionally not provided.
- **Ownership model:** the graph owns every object. `owns<>` and `uses<>` both pass references; `owns<>` additionally enforces a single owner and documents that the part belongs to that object. Objects should store the references they receive and never copy the graph (copy and move are deleted).
- **Constructors:** objects with relations need a constructor taking the related objects in written order; aggregates without constructors work only without relations. Values that are not graph objects belong in a configuration object (a plain type with default member initializers) referenced through `init_with<>`.
- **Failure handling:** constructors cannot report failure, so anything that can fail belongs in `init()`. During rollback the `deinit()` results are ignored and the `init()` failure is returned. `deinit()` always visits every object.
- **Composite objects:** an object that is itself a `composite` is one object for the graph: its leaves get no hooks from the graph, and `objects().for_each_child()` does not expand it (`objects().for_each()` would).
- **Thread safety:** none. Call `init()` and `deinit()` from one context; concurrent access to the objects needs application-level synchronization.
- **Complexity:** `init()` and `deinit()` are O(N) in the number of objects. All relation checks run at compile time and cost nothing at runtime.
