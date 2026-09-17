# `castle/events/sigslot.h` — Fixed-capacity signal and slot

Header: `castle/events/sigslot.h`
Namespace: `castle::sigslot`
Sample: `samples/sample_sigslot.cpp`

## Purpose

`signal<MaxSlot, Signature, StorageSize, StorageAlignment>` is a fixed-capacity signal/slot mechanism for event-driven applications.

It provides a Qt-like programming model:

```cpp
signal.connect(slot);
signal.emit(args...);
```

while keeping the design suitable for embedded systems:

* No dynamic memory allocation.
* No `std::function`.
* No virtual callback interface.
* Fixed maximum number of connected slots.
* Callback objects are stored inline.
* Connection lifetime is managed by RAII.
* Signal invocation is bounded by a compile-time capacity.
* Slots are invoked in deterministic registration order.

A signal represents a notification with one specific function signature.

For example:

```cpp
signal<8, void(std::uint16_t)> speed_changed;
```

means that `speed_changed` can have at most eight connected slots and every slot must be callable as:

```cpp
void(std::uint16_t)
```

A signal is therefore different from an event dispatcher.

An event dispatcher usually manages different event types:

```text
Event A -> callbacks
Event B -> callbacks
Event C -> callbacks
```

A signal represents one strongly typed notification:

```text
speed_changed
      |
      +--> slot 0
      +--> slot 1
      +--> slot 2
      +--> ...
```

## Design notes

### Fixed-capacity

The maximum number of connections is part of the signal type:

```cpp
castle::sigslot::signal<
    8,
    void(std::uint16_t)> speed_changed;
```

The signal owns storage for exactly eight slots.

There is no dynamically growing container.

This makes the resource budget explicit:

```text
signal<8, void(uint16_t)>
       |
       +-- maximum 8 connections
       |
       +-- fixed callback storage
```

Use a capacity that represents the real architectural limit of the system rather than choosing an unnecessarily large value.

### Inline callback storage

Each connected callback is stored using `inplace_function`.

This allows stateful lambdas and functors to be stored without heap allocation.

For example:

```cpp
std::uint32_t counter = 0U;

auto connection =
    signal.connect(
        [&counter](std::uint16_t)
        {
            ++counter;
        });
```

The lambda object itself is stored inside the signal's inline storage.

The lambda contains the captured reference/pointer to `counter`.

The signal does not own `counter`.

Therefore:

```text
signal
  |
  +-- inplace_function
        |
        +-- lambda object
              |
              +-- reference/pointer -> counter
```

The lifetime of objects referenced by a captured reference or pointer remains the programmer's responsibility.

### No virtual callback dispatch

Signal invocation uses the type-erased function pointer mechanism provided by `inplace_function`.

There is no virtual `slot` base class and no virtual callback function.

This keeps the callback abstraction compatible with systems that avoid virtual functions.

### One signature per signal

All slots connected to the same signal must have the same signature.

For example:

```cpp
castle::sigslot::signal<
    8,
    void(std::uint16_t)> speed_changed;
```

accepts:

```cpp
void(std::uint16_t);
```

but not:

```cpp
void(std::uint8_t);
void();
void(std::uint16_t, std::uint8_t);
```

This makes the signal interface strongly typed and allows the compiler to reject incompatible callbacks.

If different notifications require different signatures, use different signals:

```cpp
castle::sigslot::signal<
    8,
    void(std::uint16_t)> speed_changed;

castle::sigslot::signal<
    4,
    void(std::uint8_t)> fault_detected;
```

### RAII connection

`connect()` returns a `connection`.

The connection represents the lifetime of the subscription:

```cpp
{
    auto connection =
        signal.connect(callback);

    signal.emit(10U);

} // automatically disconnects
```

A connection is move-only.

This prevents multiple connection objects from accidentally representing ownership of the same subscription.

Explicit disconnection is also available:

```cpp
auto connection =
    signal.connect(callback);

connection.disconnect();
```

### Signal lifetime

A connection does not own the signal.

If the signal is destroyed, all outstanding connections are invalidated.

This prevents a connection from later attempting to access a destroyed signal.

The relationship is therefore:

```text
signal
  |
  +-- slot
       |
       +-- inplace_function
       
connection
  |
  +-- identifies one slot
```

The connection does not own the callback or signal.

### Deterministic invocation order

Slots are invoked in slot order.

For example:

```cpp
auto first =
    signal.connect(first_callback);

auto second =
    signal.connect(second_callback);

auto third =
    signal.connect(third_callback);

signal.emit(10U);
```

produces:

```text
first_callback(10)
second_callback(10)
third_callback(10)
```

The implementation performs a bounded traversal of the fixed-capacity slot storage.

The worst-case number of slot checks is therefore bounded by `MaxSlot`.

### Connection identity

Each slot has a generation counter.

A connection identifies a slot using:

```text
slot index + generation
```

When a slot is disconnected, its generation is incremented.

This prevents an old connection from accidentally disconnecting a new subscription that happens to reuse the same slot.

For example:

```text
slot 2
generation = 5
    |
    +-- connection A

connection A disconnects

slot 2
generation = 6
    |
    +-- new connection B
```

Connection A can no longer affect connection B.

## API

| Member                                                      | Notes                                                        |
| ----------------------------------------------------------- | ------------------------------------------------------------ |
| `signal<MaxSlot, Signature, StorageSize, StorageAlignment>` | Fixed-capacity signal                                        |
| `connect(callback)`                                         | Connect a lambda, free function, static function, or functor |
| `connect(object, member_function)`                          | Connect an object member function                            |
| `emit(args...)`                                             | Invoke all connected slots                                   |
| `operator()(args...)`                                       | Equivalent to `emit(args...)`                                |
| `disconnect_all()`                                          | Disconnect every slot                                        |
| `size()`                                                    | Number of active connections                                 |
| `capacity()`                                                | Maximum number of connections                                |
| `empty()`                                                   | Returns `true` when there are no connections                 |
| `connection::disconnect()`                                  | Disconnect one subscription                                  |
| `connection::connected()`                                   | Check whether the subscription is still active               |

A signal currently supports `void` return types only.

This is intentional.

A signal can fan out to multiple slots, therefore there is no single meaningful return value:

```cpp
slot_1() -> ?
slot_2() -> ?
slot_3() -> ?
```

If a computation needs one return value, use a normal function instead.

## Diagram — signal / slot lifetime

```text
                         connect()
                            |
                            v
                    +---------------+
                    |    signal     |
                    +---------------+
                            |
             +--------------+--------------+
             |              |              |
             v              v              v
         +-------+      +-------+      +-------+
         | Slot 0|      | Slot 1|      | Slot 2|
         +-------+      +-------+      +-------+
             |              |              |
             v              v              v
       inplace_function  inplace_function  inplace_function
             |              |              |
             v              v              v
          callback A     callback B      callback C


       connection A
             |
             +---- identifies Slot 0

       connection B
             |
             +---- identifies Slot 1
```

When a connection is destroyed:

```text
connection B
     |
     v
disconnect Slot 1
     |
     +--> callback B destroyed
     +--> Slot 1 becomes available
     +--> generation incremented
```

When the signal is destroyed:

```text
signal destructor
       |
       +--> invalidate all connections
       |
       +--> destroy callback objects
       |
       +--> release fixed storage
```

## Example

### Basic signal and slot

```cpp
#include <castle/events/sigslot.h>

#include <cstdint>

class Vehicle
{
public:

    void on_speed_changed(std::uint16_t speed)
    {
        // Handle speed change.
    }

    void on_fault(std::uint8_t code)
    {
        // Handle fault.
    }
};

class VehicleController
{
public:

    castle::sigslot::signal<
        8,
        void(std::uint16_t)> speed_changed;

    castle::sigslot::signal<
        4,
        void(std::uint8_t)> fault_detected;
};

int main()
{
    Vehicle vehicle;
    VehicleController controller;

    auto speed_connection =
        controller.speed_changed.connect(
            vehicle,
            &Vehicle::on_speed_changed);

    auto fault_connection =
        controller.fault_detected.connect(
            vehicle,
            &Vehicle::on_fault);

    controller.speed_changed.emit(120U);
    controller.fault_detected.emit(3U);
}
```

The important concept is:

```text
speed_changed
      |
      +--> Vehicle::on_speed_changed(uint16_t)

fault_detected
      |
      +--> Vehicle::on_fault(uint8_t)
```

Each signal has its own signature and capacity.

### Lambda slot

A lambda can be connected directly:

```cpp
auto connection =
    controller.speed_changed.connect(
        [](std::uint16_t speed)
        {
            // Handle speed.
        });
```

The lambda is stored inside the signal.

No heap allocation is required.

### Stateful lambda

Captured state is also supported:

```cpp
std::uint32_t counter = 0U;

auto connection =
    controller.speed_changed.connect(
        [&counter](std::uint16_t)
        {
            ++counter;
        });

controller.speed_changed.emit(100U);
controller.speed_changed.emit(110U);

// counter == 2U
```

The signal owns the lambda object, but it does not own `counter`.

Therefore `counter` must outlive the connection.

A safer alternative when ownership should be contained inside the callback is capture-by-value:

```cpp
auto connection =
    controller.speed_changed.connect(
        [counter = std::uint32_t{0U}]
        (std::uint16_t) mutable
        {
            ++counter;
        });
```

The captured `counter` is then part of the lambda object stored by the signal.

### Free function

A free function can be connected directly:

```cpp
void on_speed(std::uint16_t speed)
{
    // ...
}

auto connection =
    controller.speed_changed.connect(
        &on_speed);
```

### Static member function

A static member function works in the same way:

```cpp
class Diagnostics
{
public:

    static void on_speed(std::uint16_t speed)
    {
        // ...
    }
};

auto connection =
    controller.speed_changed.connect(
        &Diagnostics::on_speed);
```

### Functor

A function object can also be used:

```cpp
class SpeedMonitor
{
public:

    void operator()(std::uint16_t speed)
    {
        // ...
    }
};

SpeedMonitor monitor;

auto connection =
    controller.speed_changed.connect(monitor);
```

The `SpeedMonitor` object is stored by value inside the signal.

### Explicit disconnect

A connection can be disconnected before its lifetime ends:

```cpp
auto connection =
    controller.speed_changed.connect(
        [](std::uint16_t)
        {
            // ...
        });

controller.speed_changed.emit(100U);

connection.disconnect();

controller.speed_changed.emit(110U);

// The callback is not called for 110U.
```

Calling `disconnect()` more than once is safe.

### Automatic disconnect

RAII can be used to scope a subscription:

```cpp
{
    auto connection =
        controller.speed_changed.connect(
            [](std::uint16_t speed)
            {
                // Temporary subscription.
            });

    controller.speed_changed.emit(100U);

} // connection automatically disconnects

controller.speed_changed.emit(110U);
```

The callback is active only while `connection` is alive.

## Notes / pitfalls

### 1. The signal does not own referenced objects

This is important when connecting member functions.

```cpp
Vehicle vehicle;

auto connection =
    controller.speed_changed.connect(
        vehicle,
        &Vehicle::on_speed_changed);
```

The signal stores the callable adapter, which contains a pointer to `vehicle`.

It does not own `vehicle`.

Therefore this is only valid while `vehicle` remains alive:

```text
Vehicle
   ^
   |
callback adapter
   ^
   |
signal
```

The recommended lifetime relationship is:

```text
Vehicle lifetime
        |
        +----------------------+
                               |
Signal / connection lifetime  |
        |                      |
        +----------------------+
```

The referenced object must outlive the connection that uses it.

### 2. Captured references have the same rule

This is unsafe:

```cpp
auto make_connection()
{
    std::uint32_t counter = 0U;

    return signal.connect(
        [&counter](std::uint16_t)
        {
            ++counter;
        });
}
```

`counter` is destroyed when `make_connection()` returns.

The lambda then contains a dangling reference.

Use a longer-lived object or capture state by value when appropriate.

### 3. Callback size is bounded

`inplace_function` uses compile-time-sized inline storage.

A callback that does not fit produces a compile-time error.

For example:

```cpp
using signal_type =
    castle::sigslot::signal<
        8,
        void(std::uint16_t),
        32>;
```

Every callback must fit into the configured inline storage.

Increasing `StorageSize` allows larger callbacks:

```cpp
using signal_type =
    castle::sigslot::signal<
        8,
        void(std::uint16_t),
        64>;
```

Remember that increasing `StorageSize` also increases the memory footprint of every slot.

### 4. `MaxSlot` is a resource limit

This:

```cpp
castle::sigslot::signal<
    8,
    void(std::uint16_t)> speed_changed;
```

does not mean "normally eight callbacks".

It means:

> This signal reserves enough storage for at most eight simultaneous subscriptions.

When all slots are occupied, a new connection fails.

Always check the returned connection when the connection operation can fail:

```cpp
castle::sigslot::signal_error error;

auto connection =
    signal.connect(
        callback,
        &error);

if (error != castle::sigslot::signal_error::ok)
{
    // Handle a full signal or invalid callback.
}
```

### 5. Signal invocation is not thread-safe

`signal` does not provide synchronization.

Concurrent calls to:

```cpp
connect()
disconnect()
emit()
```

require external synchronization.

This is intentional.

Adding locks or atomics to the basic signal would increase both runtime cost and complexity for applications that do not need them.

For interrupt-driven systems, carefully define whether a signal is allowed to be emitted from an ISR before using it there.

### 6. Do not use signal as a queue

A signal performs synchronous notification:

```cpp
signal.emit(100U);

callback_a(100U);
callback_b(100U);
callback_c(100U);

signal.emit() returns;
```

It does not enqueue work.

If the producer and consumer need temporal decoupling, use an appropriate queue/event mechanism instead.

A useful rule is:

```text
Signal       -> "notify these listeners now"

Event queue  -> "deliver this event later"
```

### 7. Callback mutation during `emit()`

Callbacks may disconnect subscriptions during signal invocation.

For example:

```cpp
auto connection_b =
    signal.connect(callback_b);

auto connection_a =
    signal.connect(
        [&connection_b](std::uint16_t)
        {
            connection_b.disconnect();
        });
```

A disconnected slot is not invoked when its turn is reached.

Applications should avoid complicated connect/disconnect operations from inside callbacks unless the behavior is explicitly part of the design.

For safety-critical code, keep signal topology stable during an emission whenever practical.

### 8. Invocation order is deterministic, but callback execution time is not

The signal guarantees a bounded traversal and deterministic slot order.

It does not make arbitrary user callbacks deterministic.

For example:

```cpp
signal.emit(100U);
```

has bounded dispatch overhead, but:

```cpp
callback_a();
callback_b();
callback_c();
```

may each perform arbitrary work.

For hard real-time systems, the execution-time budget of connected callbacks must still be analyzed.

## Choosing between signal and callback registry

| Property                    | `signal`           | `callback_registry`           | `inplace_callback_registry`     |
| --------------------------- | ------------------ | ----------------------------- | ------------------------------- |
| Main abstraction            | Signal / slot      | Generic pub/sub               | Owning pub/sub                  |
| Callback ownership          | Signal             | Caller                        | Registry                        |
| Dynamic allocation          | No                 | No                            | No                              |
| Fixed capacity              | Yes                | Yes                           | Yes                             |
| Stateful lambda             | Yes                | External lifetime required    | Yes                             |
| RAII connection             | Yes                | Subscription handle           | Subscription handle             |
| Member function convenience | Yes                | Through callback object       | Through callable                |
| Qt-like API                 | Yes                | No                            | No                              |
| Virtual callback interface  | No                 | Existing registry abstraction | Existing registry abstraction   |
| Best use                    | Typed notification | Low-level callback fan-out    | Generic owning callback storage |

Use `signal` when the API itself represents a named notification:

```cpp
controller.speed_changed
controller.fault_detected
sensor.temperature_changed
```

Use `callback_registry` when the primary requirement is generic callback registration.

Use `inplace_callback_registry` when generic callback ownership is required without heap allocation.

## Signal versus event

Signals and events solve related but different problems.

A signal is usually associated directly with a source:

```cpp
controller.speed_changed.emit(120U);
```

The notification happens synchronously.

An event system is useful when events need to be represented and dispatched independently:

```text
Producer
   |
   v
Event
   |
   v
Dispatcher
   |
   +--> Handler A
   +--> Handler B
```

A practical guideline:

```text
Use signal when:
    "Something happened; notify these listeners now."

Use event when:
    "Represent this occurrence and dispatch it through
     the application's event mechanism."
```

## Recommended usage pattern

For embedded applications, prefer declaring signals as part of the public interface of the component that produces the notification:

```cpp
class VehicleController
{
public:

    castle::sigslot::signal<
        8,
        void(std::uint16_t)> speed_changed;

    castle::sigslot::signal<
        4,
        void(std::uint8_t)> fault_detected;
};
```

Then consumers establish their subscriptions during initialization:

```cpp
auto speed_connection =
    controller.speed_changed.connect(
        vehicle,
        &Vehicle::on_speed_changed);

auto fault_connection =
    controller.fault_detected.connect(
        vehicle,
        &Vehicle::on_fault);
```

Keep the connection objects alive for as long as the subscription is required.

This makes the lifetime of the relationship explicit in the owning component.

## See also

* `castle/callbacks/inplace_function.h` — Type-erased callable with inline storage and no heap allocation.
* `castle/callbacks/callback_registry.h` — Non-owning fixed-capacity callback registry.
* `castle/callbacks/inplace_callback_registry.h` — Owning fixed-capacity callback registry.
* `castle/events/event_dispatcher.h` — Event dispatching for multiple event types.
* `castle/events/inplace_event_dispatcher.h` — Heap-free event dispatcher.
