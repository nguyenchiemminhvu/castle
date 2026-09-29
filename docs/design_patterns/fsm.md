# Finite State Machine (FSM)

## Overview
A compile-time described finite state machine for embedded applications. States and events are C++ types, and a transition table maps an exact `(source state, event type)` pair to a target state. Optional guard and action functors provide decision and side-effect behavior without virtual dispatch or callback type erasure.

## Header
`#include "castle/design_patterns/fsm.hpp"`

## Dependencies
- `castle/core/compiler.hpp`
- `castle/core/traits.hpp`
- `castle/core/types.hpp`

No dynamic container, clock, event dispatcher, callback wrapper, or STL header is required.

## Public API

| API | Description |
| --- | --- |
| `fsm::empty_context` | Empty context passed to guards/actions by the one-argument `process_event()` overload. |
| `fsm::always_guard` | Default guard; accepts every event. |
| `fsm::no_action` | Default action; does nothing. |
| `fsm::transition<From, Event, To, Guard, Action>` | Compile-time transition row. `Guard` defaults to `always_guard`; `Action` defaults to `no_action`. |
| `fsm::event_result` | Result enum: `transitioned`, `ignored`, or `guard_rejected`. |
| `fsm::state_machine<Initial, Rows...>` | FSM type whose states are inferred from the initial state and all transition rows. |
| `state_machine::state_count` | Number of unique state types in the machine. |
| `state_machine::transition_count` | Number of transition rows. |
| `state_machine::state_id<State>()` | Compile-time state index; rejects state types not present in the machine. |
| `state_machine::current_state_id()` | Runtime index for the current state. |
| `state_machine::is_in_state<State>()` | Tests the current state using a compile-time state type. |
| `state_machine::has_transition<State, Event>()` | Compile-time query for a declared state/event pair. Event cv/ref qualifiers are removed. |
| `state_machine::process_event(event, context)` | Processes an event with caller-owned application context. |
| `state_machine::process_event(event)` | Processes an event with a temporary `empty_context`. |
| `state_machine::reset()` | Restores the initial state without running a transition action. |

### Transition declaration

```cpp
using machine = castle::design_patterns::fsm::state_machine<
    idle,
    castle::design_patterns::fsm::transition<
        idle, start_event, running, start_guard, start_action
    >,
    castle::design_patterns::fsm::transition<running, stop_event, idle>
>;
```

Template parameter order is `SourceState, Event, TargetState, Guard, Action`. A guard is evaluated first. When it returns `true`, the action runs and then the destination state is committed.

## Usage Example

```cpp
#include "castle/design_patterns/fsm.hpp"
#include <cassert>

namespace fsm = castle::design_patterns::fsm;

struct idle {};
struct running {};
struct start_event {};

struct application_context
{
    bool start_allowed = true;
    unsigned start_count = 0U;
};

struct start_guard
{
    bool operator()(application_context& context, start_event const&) const noexcept
    {
        return context.start_allowed;
    }
};

struct start_action
{
    void operator()(application_context& context, start_event const&) const noexcept
    {
        ++context.start_count;
    }
};

using machine = fsm::state_machine<
    idle,
    fsm::transition<idle, start_event, running, start_guard, start_action>
>;

int main()
{
    machine controller;
    application_context context{};

    assert(controller.is_in_state<idle>());
    assert(controller.process_event(start_event{}, context) == fsm::event_result::transitioned);
    assert(controller.is_in_state<running>());
    assert(context.start_count == 1U);
}
```

See `samples/design_patterns/fsm.cpp` for a complete standalone example covering guards, actions, multiple source states for one event type, the empty-context overload, reset, and state metadata.

## Constraints & Notes

- **No runtime state objects:** State types are tags. Put mutable application data in the context passed to `process_event()`.
- **Exact event matching:** The selected row's event type must exactly match the event's decayed type. There is no inheritance-based or runtime-polymorphic matching.
- **Unique keys:** A machine may declare only one row for each `(source state, event type)` pair. This keeps dispatch deterministic. A false guard returns `guard_rejected`; it does not fall through to another row.
- **Guard/action requirements:** Both functor types must be nothrow default-constructible and callable as `functor(context, event)`. Their invocation must be `noexcept`. The functor objects are temporary per event; keep persistent application data in `context`.
- **Results:** `transitioned` means the guard passed, the action ran, and the destination was committed (including a self-transition). `ignored` means there was no row for the current state/event pair. `guard_rejected` means a matching row existed but its guard returned false.
- **Self-transitions:** A row may target its own source state. Its guard and action still execute and the result is `transitioned`.
- **Reset:** `reset()` only changes the state index. It does not call guards, actions, entry hooks, or exit hooks.
- **Storage:** Each instance stores one `castle::size_type` state index. There is no heap allocation, RTTI, virtual dispatch, exception handling, or STL dependency.
- **Complexity:** State queries and reset are O(1). Event dispatch checks at most the declared number of transition rows, so processing is O(T) for T rows. Template expansion and state-list construction happen at compile time.
- **Thread-safety:** A machine instance has no internal synchronization. External synchronization is required when multiple tasks or interrupt contexts can access the same instance.
- **State IDs:** IDs are assigned in compile-time encounter order: initial state first, then each row's source and target state if not already seen. Treat IDs as machine-local diagnostics, not stable serialized identifiers across transition-table edits.
