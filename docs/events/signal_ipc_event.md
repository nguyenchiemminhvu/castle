# signal_ipc_event

## Overview
`castle::events::signal_ipc_event` binds a fixed set of POSIX signals to fixed-capacity callback registries. It exists for processes that need immediate signal-context dispatch with Castle-owned inline storage and no Castle heap allocation.

## Header
`#include "castle/events/signal_ipc_event.hpp"`

## Dependencies
- [`signal_ipc_config`](signal_ipc_config.md)
- [`function`](../callbacks/function.md)
- [`function_registry`](../callbacks/function_registry.md)
- [`subscription`](../callbacks/subscription.md)
- [`tuple`](../utility/tuple.md)
- [`bitset`](../utility/bitset.md)
- [`atomic`](../atomic/atomic.md)
- [`status`](../error/status.md)

## Public API
| API | Description |
| --- | --- |
| `template <typename... SignalConfigs> class signal_ipc_event` | Static dispatcher type. `SignalConfigs...` must be unique `signal_ipc_config<...>` instantiations. |
| `using error = castle::status` | Error type used by `install()` and `register_callback()`. |
| `using subscription = callbacks::subscription` | Unsubscribe handle returned by `register_callback()`. |
| `static constexpr size_type signal_count` | Number of configured signals. |
| `static constexpr size_type signal_capacity()` | Returns `signal_count`. |
| `template <signal Signum> static constexpr size_type callback_capacity()` | Returns the per-signal callback capacity. |
| `template <signal Signum> static constexpr size_type callback_storage_size()` | Returns the per-signal inline callback storage size. |
| `template <signal Signum> static constexpr size_type callback_storage_alignment()` | Returns the per-signal inline callback storage alignment. |
| `static error install()` | Installs the class handler with `sigaction()` for every configured signal and enables all of them. Returns `status::system_call_error` on failure. |
| `static void uninstall()` | Restores `SIG_DFL` for every configured signal. Registered callbacks remain stored. |
| `static bool is_installed()` | Returns whether the internal ready gate is published as installed. |
| `template <signal Signum, typename Callable> static subscription register_callback(Callable&&, error* = nullptr)` | Stores a `void()` callback by value for `Signum`. The callback executes directly from signal context. |
| `template <signal Signum> static void enable_signal()` | Enables callback invocation for `Signum`. |
| `template <signal Signum> static void disable_signal()` | Disables callback invocation for `Signum` while keeping the handler installed. |
| `template <signal Signum> static bool is_signal_enabled()` | Returns whether `Signum` is currently enabled. |
| `template <signal Signum> static void clear_signal()` | Clears all subscribers for `Signum`. |
| `static void clear()` | Clears every configured signal registry. |
| `template <signal Signum> static size_type subscriber_count()` | Returns the active subscriber count for `Signum`. |

## Usage Example
See [`samples/sample_signal_ipc_event.cpp`](../../samples/sample_signal_ipc_event.cpp).

```cpp
using namespace castle::events;
using ipc = signal_ipc_event<
    signal_ipc_config<signal::sigusr1, 2U>,
    signal_ipc_config<signal::sigusr2, 1U>
>;

volatile sig_atomic_t usr1_count = 0;
ipc::register_callback<signal::sigusr1>([&usr1_count]() { ++usr1_count; });

ipc::install();
::raise(SIGUSR1);
ipc::uninstall();
```

## Constraints & Notes
- Callback signature is always `void()`. Signal identity is selected by the `Signum` template argument.
- Castle-owned storage is entirely static and fixed at compile time; Castle performs no heap allocation.
- User callbacks must be async-signal-safe. The callback body runs inside the installed POSIX handler.
- `install()` publishes prior registrations to the handler through a release/acquire gate. Registrations or clears performed after `install()` are not synchronized with in-flight delivery.
- `enable_signal()` and `disable_signal()` only gate callback invocation; they do not stop the OS from delivering the signal to the installed handler.
- `clear_signal()` and `clear()` make existing `subscription` objects stale. A later `unsubscribe()` on such a handle returns `status::invalid_subscription`.
