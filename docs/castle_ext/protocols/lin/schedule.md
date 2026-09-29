# LIN Schedule Table

## Overview

Stores an ordered, cyclic list of LIN master schedule slots in compile-time-bounded inline storage. Each slot specifies a frame identifier, response owner, and slot duration in application-defined ticks.

## Header

`#include "castle_ext/protocols/lin/schedule.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`frame.hpp`](frame.md)

## Public API

| API | Description |
|---|---|
| `schedule_entry` | Slot value with `identifier`, `owner`, `slot_ticks`, and `valid()`. |
| `schedule_table<Capacity>` | Fixed-capacity schedule; `Capacity` must be greater than zero. |
| `size()`, `capacity()`, `empty()` | Query current size, compile-time capacity, and emptiness. O(1). |
| `add(id, owner, slot_ticks)` | Appends a slot; returns `full` or `invalid_argument` when rejected. O(1). |
| `get(index, output)` | Copies a slot by index; returns `out_of_range` for an invalid index. O(1). |
| `next(output)` | Returns the next slot and wraps the cursor; returns `empty` if not configured. O(1). |
| `reset()` | Rewinds the cyclic cursor. O(1). |
| `clear()` | Removes all slots and resets the cursor. O(1). |

## Usage Example

```cpp
#include "castle_ext/protocols/lin/schedule.hpp"
#include <assert.h>

int main()
{
    using namespace castle::protocols::lin;
    schedule_table<4U> schedule;
    assert(castle::succeeded(schedule.add(0x10U, response_owner::slave, 5U)));
    schedule_entry current{};
    assert(castle::succeeded(schedule.next(current)));
    assert(current.identifier == 0x10U);
    return 0;
}
```

See [`samples/castle_ext/protocols/lin/schedule.cpp`](../../../../samples/castle_ext/protocols/lin/schedule.cpp).

## Constraints & Notes

No timer or delay is executed internally; `slot_ticks` is interpreted by the application's timer/event loop. Duplicate identifiers are allowed because schedule tables may repeat frames. Mutation and cursor access are not internally synchronized.
