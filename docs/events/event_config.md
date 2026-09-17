# `castle/events/event_config.h` — Event slot descriptor

**Header:** `castle/events/event_config.h`
**Namespace:** `castle::events`

## Purpose

`event_config<EventTag, MaxCallback, Signature, StorageSize,
StorageAlignment>` is a **compile-time descriptor** for a single event
slot. The same descriptor is consumed by both:

- `event_dispatcher<Configs...>` — non-owning (reads `EventTag`,
  `MaxCallback`, `Signature`).
- `inplace_event_dispatcher<Configs...>` — owning / SBO (also reads
  `StorageSize` and `StorageAlignment`).

Switching between the two flavours therefore only requires changing
the outer class name; the pack of configs is reused verbatim.

## Template parameters

| Parameter                     | Meaning                                          |
| ----------------------------- | ------------------------------------------------ |
| `EventTag`                    | Compile-time key type (an empty struct is fine)  |
| `MaxCallback`                 | Max concurrent subscribers for this event        |
| `Signature`                   | Callback signature `void(Args...)`               |
| `CallbackStorageSize = 64`    | Inline buffer size (inplace variant only)        |
| `CallbackStorageAlignment`    | Inline buffer alignment (inplace variant only)   |

## Example

```cpp
#include <castle/events/event_dispatcher.h>
#include <castle/events/inplace_event_dispatcher.h>

struct TimerExpired {};
struct VehicleStarted {};
struct Shutdown {};

using configs = std::tuple<
    castle::events::event_config<TimerExpired,   8, void(std::uint32_t, std::uint32_t)>,
    castle::events::event_config<VehicleStarted, 4, void(std::uint8_t)>,
    castle::events::event_config<Shutdown,       2, void(), 32>
>;
// Actual usage: pass configs as a parameter pack, not as a tuple:
using dispatcher_t         = castle::events::event_dispatcher<
    castle::events::event_config<TimerExpired,   8, void(std::uint32_t, std::uint32_t)>,
    castle::events::event_config<VehicleStarted, 4, void(std::uint8_t)>,
    castle::events::event_config<Shutdown,       2, void(), 32>>;
```

## See also

- `event_dispatcher.h`, `inplace_event_dispatcher.h` — consumers.
- `signal_config.h` — sister descriptor for POSIX signal handling.
