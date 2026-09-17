# Callback execution policies

## Overview

Header-only policies that decide when a stored callback should run. The design is poll-driven for timing policies and supports single-thread and atomic concurrent variants.

## Header

```cpp
#include "castle/callbacks/callback_policy.h"
```

**Namespace:** `castle::callbacks::callback_policy`

## Main API

| API | Purpose |
|---|---|
| Policy objects | Bind a callback once and call `execute(...)` when the application wants to test the policy. |
| `single_thread` / `concurrent` | Single-thread state and atomic state variants are provided where the policy supports both. |
| `on_change` | Run when the incoming value changes; keeps the last value in an optional. |
| `throttle` | Rate-limit calls: extra calls inside the interval are dropped. |
| Time-window policies | The header also contains timing/state policies built on Castle chrono clocks. |
| Factory helpers | Small `make_policy_*` helpers deduce callable and value types. |

## Example

```cpp
auto policy = castle::callbacks::callback_policy::on_change::make_policy_st<int>(
    [](int v) { /* react to change */ });
policy.execute(10);
policy.execute(10); // no second callback for the same value
```

## Embedded notes

- No thread or timer is created by these policies.
- Time-based policies are checked when `execute()` is called.
- The concurrent variants use Castle atomics rather than a heap-allocated lock.
