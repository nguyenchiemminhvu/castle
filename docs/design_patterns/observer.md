# Observer

## Overview
Fixed-capacity observer interfaces and an observable registry for embedded event fan-out. Use it when a subject must notify a bounded set of listeners without dynamic allocation.

## Header
`#include "castle/design_patterns/observer.hpp"`

## Dependencies
- [`../container/array.md`](../container/array.md)
- `castle/core/types.hpp`
- `castle/core/traits.hpp`

## Public API
| API | Description |
| --- | --- |
| `observer<T>` | Abstract observer interface with `notify(T const&)`. |
| `observer<void>` | Abstract observer interface for parameterless notifications. |
| `observer<T, Rest...>` | Multiple-inheritance helper that exposes all `notify()` overloads for unique payload types. |
| `observable<TObserver, N>` | Stores up to `N` observer pointers in fixed-capacity inline storage. |
| `add_observer` | Registers a non-null observer pointer; rejects duplicates and overflow. |
| `remove_observer` | Unregisters a previously added observer pointer. |
| `notify_observers` | Invokes `notify(data)` or `notify()` on every registered observer. |

## Usage Example
```cpp
// See: samples/sample_observer.cpp
#include "castle/design_patterns/observer.hpp"

struct temperature_observer : castle::design_patterns::observer<uint16_t>
{
    void notify(uint16_t const& value) override { last = value; }
    uint16_t last = 0U;
};
```

## Constraints & Notes
- No heap allocation; `observable` stores observer pointers in `castle::container::array`.
- Lifetime management is manual: observers must outlive their registrations.
- Duplicate registrations are rejected.
- Dispatch uses virtual functions, so this header does not avoid virtual dispatch even though it remains RTTI-free.
