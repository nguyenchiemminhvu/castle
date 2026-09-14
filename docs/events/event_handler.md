# castle::events::event_handler

## Overview

`castle::events::event_handler` is a fixed-capacity asynchronous event loop for POSIX pthread based targets.

It provides the familiar event-posting model of the original `libeventcpp::event_handler` while preserving CASTLE 2.0 design goals:

- no dynamic memory allocation by the event handler
- no STL dependency
- no exceptions
- no RTTI
- no virtual dispatch
- fixed event capacity
- thread-safe event posting
- monotonic delayed scheduling

The event handler is enabled when `CASTLE_USING_PTHREAD` is enabled in `castle/core/config.hpp`.

## Header

```cpp
#include "castle/events/event_handler.hpp"
```

## Namespace

```cpp
namespace castle
{
namespace events
{
// event_handler
}
}
```

## Design

The handler owns one dedicated POSIX `pthread` and a bounded event table.

Each accepted event occupies one event slot and one entry in the scheduling heap. Events are identified internally by a slot index, so the callback storage itself is not copied into the heap.

The implementation uses:

- `castle::container::min_heap` for timestamp ordering
- `castle::container::ring_buffer` for free event-slot management
- `castle::callbacks::inplace_function` for callback storage
- `castle::chrono::steady_clock::now()` for the event timestamp source
- `pthread_mutex_t` and `pthread_cond_t` for synchronization

The scheduling heap is bounded by `MaxEventCount`. A repeated event reuses its original slot and is rescheduled after each invocation; `N` repetitions therefore consume one event slot instead of `N` slots.

## Template parameters

```cpp
template <
    castle::size_type MaxEventCount,
    castle::size_type StorageSize = castle::inplace_storage_reserved,
    castle::size_type StorageAlignment = castle::inplace_alignment_default>
class event_handler;
```

### `MaxEventCount`

Maximum number of events that can be pending or executing through the handler's fixed event storage.

### `StorageSize`

Inline storage available for the event callback and its bound arguments.

### `StorageAlignment`

Alignment of the inline callback storage.

## Basic usage

```cpp
#include "castle/events/event_handler.hpp"

castle::events::event_handler<16> handler;

handler.post_event([] {
    // Execute asynchronously.
});

handler.post_delayed_event(100U, [] {
    // Execute after approximately 100 ms.
});

handler.post_repeated_event(5U, 250U, [] {
    // Execute five times at a 250 ms interval.
});
```

## Events with arguments

The event API accepts callables with a `void(Args...)` invocation signature. Arguments are copied or moved into the fixed event object when the event is posted.

This preserves the useful part of the original `libeventcpp` API without requiring `std::function` or `std::tuple`.

```cpp
static void process_message(uint32_t id, uint16_t length)
{
    // ...
}

handler.post_event(&process_message, 42U, 128U);
handler.post_delayed_event(50U, &process_message, 42U, 128U);
handler.post_repeated_event(10U, 100U, &process_message, 42U, 128U);
```

The same form works with capturing lambdas and function objects as long as the final bound callable fits within `StorageSize`.

## Typed callback wrappers

The callback signature is part of the callable type. For example, a user-defined callback object or function wrapper implementing `void(uint32_t)` can be submitted through the same argument-bearing API:

```cpp
castle::callbacks::function<void(uint32_t)> callback(&process_message);

handler.post_event(callback, 42U);
```

The event handler itself converts the callback plus arguments into one internal `inplace_function<void()>` event task. Therefore `StorageSize` applies to the complete bound event object, including the callable and its stored arguments.

## Member functions

Member functions can be posted without shared ownership, RTTI, or `std::enable_shared_from_this`.

```cpp
class worker
{
public:
    void process(uint32_t value)
    {
        // ...
    }
};

worker object;
handler.post_event(&worker::process, &object, 42U);
handler.post_delayed_event(100U, &worker::process, &object, 42U);
handler.post_repeated_event(5U, 250U, &worker::process, &object, 42U);
```

The object must remain alive until the corresponding event has finished executing. The handler does not extend the object lifetime.

## Immediate events

```cpp
template <typename Callable>
castle::status post_event(Callable&& callable);

template <typename Callable, typename... Args>
castle::status post_event(Callable&& callable, Args&&... args);
```

The event is made runnable immediately and is executed asynchronously by the handler thread.

## Delayed events

```cpp
template <typename Callable>
castle::status post_delayed_event(
    uint64_t delay_ms,
    Callable&& callable);

template <typename Callable, typename... Args>
castle::status post_delayed_event(
    uint64_t delay_ms,
    Callable&& callable,
    Args&&... args);
```

The event is scheduled relative to `castle::chrono::steady_clock::now()`.

## Repeated events

```cpp
template <typename Callable>
castle::status post_repeated_event(
    castle::size_type times,
    uint64_t duration_ms,
    Callable&& callable);

template <typename Callable, typename... Args>
castle::status post_repeated_event(
    castle::size_type times,
    uint64_t duration_ms,
    Callable&& callable,
    Args&&... args);
```

The first invocation is scheduled immediately, matching the original `libeventcpp::post_repeated_event()` behavior.

A repeated event is rescheduled from its previous due timestamp rather than from the completion time of its callback. If callback execution takes longer than the configured interval, the next occurrence becomes ready immediately instead of accumulating additional drift.

A `times` value of zero performs no scheduling and returns `castle::status::ok`.

## Return status

The posting functions return `castle::status`.

| Status | Meaning |
| --- | --- |
| `castle::status::ok` | Event accepted and scheduled. |
| `castle::status::full` | No event slot is available. |
| `castle::status::invalid_argument` | Invalid member-function event target. |
| `castle::status::invalid_callback` | Callback is empty. |
| `castle::status::not_configured` | pthread-backed handler was not initialized. |
| `castle::status::system_call_error` | A pthread synchronization operation failed. |

## Capacity and state

```cpp
static constexpr castle::size_type capacity();
static constexpr castle::size_type callback_storage_size();
static constexpr castle::size_type callback_storage_alignment();

bool initialized() const;
bool running() const;
bool is_running() const;
castle::size_type pending() const;
castle::size_type available() const;
```

`pending()` reports scheduled events currently held by the scheduling heap. `available()` reports free event slots.

## Shutdown

```cpp
castle::status shutdown();
castle::status stop();
```

Shutdown is idempotent. It stops the event loop, wakes the worker, joins the pthread, and only then releases callback storage.

A callback must not call `shutdown()` on its own handler thread because a thread cannot safely `pthread_join()` itself. In that case `shutdown()` reports `castle::status::system_call_error` while the loop remains on the normal path toward termination.

Pending events are discarded during shutdown.

## Timing model

The event handler deliberately uses the CASTLE chrono implementation rather than issuing direct `clock_gettime()` calls for event timestamps:

```cpp
castle::chrono::steady_clock::now()
```

This keeps the time source centralized in `castle/chrono`, allowing the platform's configured steady-clock backend to be used consistently.

The pthread condition variable is configured for `CLOCK_MONOTONIC`, and delayed-event deadlines are converted from the CASTLE steady-clock time point to the POSIX `timespec` representation required by `pthread_cond_timedwait()`.

For POSIX targets, the configured CASTLE steady-clock implementation should therefore represent the same monotonic time domain used by the pthread condition variable.

## Threading model

Event callbacks always execute on the handler's dedicated pthread.

Posting is thread-safe and may be performed from other threads, including from within the event callback itself.

Callback execution occurs outside the handler mutex. A long-running callback therefore does not prevent other producer threads from acquiring the event queue mutex to post work.

## Memory model

No heap allocation is required by `castle::events::event_handler`.

The following storage is owned by the handler object:

- one pthread handle
- `MaxEventCount` callback/event slots
- one fixed-capacity free-slot ring buffer
- one fixed-capacity scheduling heap
- one pthread mutex
- one pthread condition variable

The total callback storage requirement is approximately proportional to:

```text
MaxEventCount * sizeof(event_slot)
```

plus the fixed scheduling/container storage generated by the template instantiations.

## Relation to pthread_pool

`castle::event::pthread_pool` is intended for parallel execution of independent tasks, while `castle::events::event_handler` is an ordered asynchronous scheduler with one event-loop thread.

They serve different execution models and are intentionally kept separate:

```cpp
#include "castle/threading/pthread_pool.hpp"
#include "castle/events/event_handler.hpp"

castle::event::pthread_pool<2U, 16U> pool;
castle::events::event_handler<16U> handler;
```

An event callback may submit work to the pthread pool when the application needs parallel processing after a scheduling decision has been made.

## Configuration

The handler requires POSIX pthread support:

```cpp
#define CASTLE_USING_PTHREAD 1
```

The normal CASTLE configuration mechanism in `castle/core/config.hpp` can provide this automatically on supported targets.

Build applications with pthread support, for example:

```text
-pthread
```

## Determinism considerations

The implementation is designed to make resource bounds explicit:

- queue/event capacity is fixed at compile time
- callback storage is fixed at compile time
- repeated events do not allocate one queue node per repetition
- event ordering is timestamp based with a sequence number for same-deadline FIFO ordering
- event posting fails explicitly when capacity is exhausted

Scheduling latency is still affected by the operating system, pthread implementation, interrupt load, and callback execution time. Fixed memory capacity does not make OS thread scheduling itself deterministic.

## Dependencies

The event handler depends only on CASTLE facilities and POSIX pthread APIs:

```text
castle/core/*
castle/error/status.hpp
castle/container/heap.hpp
castle/container/ring_buffer.hpp
castle/chrono/chrono.hpp
castle/utility/forward.hpp
castle/utility/move.hpp
castle/utility/tuple.hpp
castle/callbacks/inplace_function.hpp
pthread.h
```

No C++ STL threading, container, smart-pointer, or exception facilities are required.
