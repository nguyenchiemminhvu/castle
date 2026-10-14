# Design principles for safety-critical embedded C++

## Overview

CASTLE stands for **Compliant Automotive Safety Template Library for Embedded**. The goal is to use modern C++ to build embedded software that is safe, predictable, testable, and efficient under strict resource limits.

This applies to automotive ECUs such as AUTOSAR controllers, ADAS, body control, battery management, clusters, and infotainment. It also applies to aerospace, railway, industrial control, robotics, and medical devices.

## Why it matters

On a PC, a failed allocation, a late task, or an unexpected exception may only crash or slow down one process. In an ECU, the same problem can be much worse:

- A control task misses its real-time deadline.
- A CAN, LIN, or Ethernet message is handled too late.
- Memory fragments, and the system fails after hours of driving.
- A driver starts before clocks or pins are ready.
- A race condition sends a wrong actuator command.
- A fault is not detected, reported, or moved to a safe state.

So the question is not only "does the code work?" The goal is code with **predictable behavior**: bounded memory, bounded time, and known failure paths.

## Questions to ask

Ask these for every module:

1. Is memory allocated only at start-up, or is it forbidden later?
2. Is execution time bounded and predictable?
3. Does the real-time path avoid exceptions?
4. Are ownership, object lifetime, and dependencies clear?
5. Can the module be tested on a PC without hardware?
6. Are communication, concurrency, and state changes safe?
7. What happens when memory, hardware, communication, or timing fails?

## Standards

Automotive projects use guidelines such as MISRA C++, AUTOSAR C++14, CERT C++, and the ISO 26262 safety process. These are not only style rules. They prevent failures that are hard to reproduce and expensive to fix on a real target.

### Examples

**MEM52-CPP: handle allocation failure.** Define a clear response when allocation fails. Often the best choice is to avoid the failure: allocate at start-up, use fixed-capacity containers, or use bounded memory pools. CASTLE containers such as [`vector`](container/containers.md) keep their data inside the object.

**MEM57-CPP: do not use default `operator new` for over-aligned types.** Alignment can fail on the target MCU or compiler even when PC tests pass. Treat memory layout and alignment as part of the design.

**OOP51-CPP: do not slice derived objects.** Copying a derived object into a base object by value drops its data and behavior. Pick the right model: value semantics, references, runtime polymorphism, or compile-time polymorphism. CASTLE uses compile-time choices by default. See [callbacks](callbacks/function.md).

## Design flow

The diagram shows how constraints lead to design decisions, and then to engineering outcomes.

```plantuml
@startuml
title From Constraints to Embedded C++ Design

' Modern Dark Theme Configuration
skinparam backgroundcolor #121212
skinparam useBetaStyle true

<style>
title {
    FontColor #FFFFFF
    FontSize 18
    FontStyle bold
    Margin 20
}
package {
    BackgroundColor #1E1E1E
    BorderColor #333333
    BorderStyle dashed
    BorderThickness 1.5
    FontColor #00ADB5
    FontSize 14
    FontStyle bold
}
component {
    BackgroundColor #2D2D2D
    BorderColor #4F4F4F
    FontColor #EEEEEE
    FontSize 12
    RoundCorner 6
    LineThickness 1
}
arrow {
    LineColor #00ADB5
    LineThickness 1.5
}
</style>

allow_mixing
skinparam rankdir TB
skinparam shadowing false

package "System Constraints" {
    component "Limited RAM / Flash" as RAM
    component "Safety requirements" as Safety
    component "Real-time deadlines" as Deadlines
    component "Hardware failures" as Failures
    component "RTOS concurrency" as RTOS
    component "Coding standards" as Standards
}

package "Design Decisions" {
    component "Memory allocation policy" as Alloc
    component "Ownership and lifetime model" as Lifetime
    component "Task communication model" as Comm
    component "Error-handling strategy" as Error
    component "State and event model" as State
    component "Hardware abstraction boundary" as HAL
}

package "Engineering Outcomes" {
    component "Deterministic behavior" as Deterministic
    component "Safe failure modes" as SafeFail
    component "Testability" as Test
    component "Static-analysis compliance" as Static
    component "Maintainability" as Maintain
    component "Traceability" as Trace
}

' Layout alignment helpers
RAM -[hidden]right-> Safety
Safety -[hidden]right-> Deadlines
Deadlines -[hidden]right-> Failures
Failures -[hidden]right-> RTOS
RTOS -[hidden]right-> Standards

Alloc -[hidden]right-> Lifetime
Lifetime -[hidden]right-> Comm
Comm -[hidden]right-> Error
Error -[hidden]right-> State
State -[hidden]right-> HAL

Deterministic -[hidden]right-> SafeFail
SafeFail -[hidden]right-> Test
Test -[hidden]right-> Static
Static -[hidden]right-> Maintain
Maintain -[hidden]right-> Trace

' Relationships
RAM --> Alloc
Safety --> Lifetime
Deadlines --> Comm
Failures --> Error
RTOS --> State
Standards --> HAL

Alloc --> Deterministic
Lifetime --> Deterministic
Comm --> SafeFail
Error --> SafeFail
State --> Static
HAL --> Static
@enduml
```

## How CASTLE applies these principles

| Concern | CASTLE approach | Read more |
|---|---|---|
| Memory | Fixed-capacity containers, no mandatory heap. | [containers](container/containers.md) |
| Failure | Status values and assertions, no exceptions in normal paths. | [error handler](core/error_handler.md) |
| Dispatch | Compile-time choices, fixed-storage callbacks. | [function](callbacks/function.md) |
| Concurrency | Explicit synchronization types, no hidden locks. | [mutex](sync/mutex.md) |
| Testing | Host-side mock backends for hardware interfaces. | [GPIO backends](castle_ext/io/gpio/backends.md) |
