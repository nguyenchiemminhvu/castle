# `castle/events/signal_config.h` — POSIX signal descriptor

**Header:** `castle/events/signal_config.h`
**Namespace:** `castle::events`

## Purpose

Two building blocks:

1. `enum class signal : int` — a **strongly-typed enumeration** of the
   POSIX signals CASTLE can manage (`sighup`, `sigint`, `sigquit`,
   `sigill`, `sigtrap`, `sigabrt`, `sigbus`, `sigfpe`, `sigusr1`,
   `sigsegv`, `sigusr2`, `sigpipe`, `sigalrm`, `sigterm`, `sigchld`,
   `sigcont`, `sigtstp`, `sigttin`, `sigttou`, `sigurg`, `sigxcpu`,
   `sigxfsz`, `sigvtalrm`, `sigprof`, `sigsys`). Each enumerator's
   underlying value is the platform's `SIG*` macro, so `to_signum(s)`
   converts losslessly to `int`.
2. `signal_config<Signal, MaxCallback, StorageSize, StorageAlignment>`
   — compile-time descriptor of one managed signal, consumed by both
   `signal_event` (non-owning) and `inplace_signal_event` (SBO).

## Template parameters

| Parameter                | Meaning                                              |
| ------------------------ | ---------------------------------------------------- |
| `Signal`                 | Which `signal` enumerator this slot handles          |
| `MaxCallback`            | Max concurrent subscribers **per signal**            |
| `StorageSize = 64`       | Inline buffer size (inplace variant only)            |
| `StorageAlignment`       | Inline buffer alignment (inplace variant only)       |

## Example

```cpp
#include <castle/events/signal_event.h>

using castle::events::signal;
using signals_t = castle::events::signal_event<
    castle::events::signal_config<signal::sigint,  4>,
    castle::events::signal_config<signal::sigterm, 1>
>;
```

## See also

- `signal_event.h`, `inplace_signal_event.h` — consumers.
- `event_config.h` — sister descriptor for tag-keyed application events.
