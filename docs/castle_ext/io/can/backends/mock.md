# CAN Mock Backend

## Overview
`mock::basic_backend<RxQueueDepth, TxQueueDepth, MaxFilters>` is a deterministic, allocation-free, in-memory CAN controller for host-side samples and unit tests. It behaves like one node on a bus whose wire is under test control: frames passed to `send()` land in a bounded queue that the test reads back with `pop_sent()`, and the test plays the rest of the bus by calling `inject()`. Because it implements the same backend contract as the SocketCAN and Zephyr backends, application code written against [`basic_bus`](../bus.md) runs unchanged on the host with `basic_bus<mock::backend>`; no CAN interface, root privileges or operating-system support is required.

## Header
`#include "castle_ext/io/can/backends/mock.hpp"`

## Dependencies
- [`../bus.md`](../bus.md)
- [`../frame.md`](../frame.md)
- [`../filter.md`](../filter.md)
- [`../types.md`](../types.md)
- [`../../../../container/mirrored_ring_buffer.md`](../../../../container/mirrored_ring_buffer.md)
- [`../../../../error/status.md`](../../../../error/status.md)

## Public API
Template parameters (all `castle::size_type`):

| Parameter | Default | Meaning |
|---|---|---|
| `RxQueueDepth` | `16U` | Frames buffered between `inject()` (or loopback) and `receive()`. |
| `TxQueueDepth` | `16U` | Frames buffered between `send()` and `pop_sent()`. |
| `MaxFilters` | `8U` | Simultaneously installed filters. |

`mock::backend` is the alias for `basic_backend<>`. `endpoint_type` is `const char*`: any non-empty string (for example `"vcan0"`). The name is not stored or interpreted.

Backend contract (used by `basic_bus`; every call clears `*system_error` to `0` on entry when the pointer is not null):

| API | Description |
|---|---|
| `open(endpoint, config, system_error)` | `ok`; `invalid_argument` for a null or empty name; `invalid_config` when already open. Stores `config`, enters `bus_state::error_active`, zeroes both error counters and both drop counters, disarms `fail_next_send()` and empties the filter table and both queues. The backend does not validate `config` itself (the facade does) and never uses the bit rate. |
| `close(system_error)` | `ok`, or `not_configured` when already closed. Enters `bus_state::stopped`, removes all filters and discards queued receive and transmit frames. `configuration()` keeps its value. |
| `is_open()` | `true` between a successful `open()` and `close()`. |
| `send(frame, timeout, system_error)` | Checked in this order: `not_configured` (closed); `invalid_config` in `can_mode::listen_only` (the armed fault, if any, stays armed); the status armed by `fail_next_send()` (one shot, counted in `tx_failures`); `system_call_error` with `system_error = ENETUNREACH` while the scripted state is `bus_off` (counted in `tx_failures`); in `can_mode::loopback` the frame is delivered to the node's own receive path and `ok` is returned; otherwise the frame is pushed to the transmit queue, giving `ok` or `full` when `TxQueueDepth` unread frames are waiting. A `full` queue is not counted as a failure. The timeout is ignored. |
| `receive(out, timeout, system_error)` | Pops the oldest delivered frame: `ok`, `empty` immediately when there is none, or `not_configured`. The timeout is ignored. |
| `add_filter(filter, handle, system_error)` | `ok` with the slot index as `handle`; `full` when `MaxFilters` filters are installed (handle is `invalid_filter`); `not_configured`. |
| `remove_filter(handle, system_error)` | `ok`; `not_found` for a handle that is not installed; `not_configured`. |
| `clear_filters(system_error)` | `ok` or `not_configured`. |
| `query_status(out, system_error)` | `not_configured` when closed (`out` is a default `bus_status`), otherwise `ok` with the scripted `state`, `tx_error_counter`, `rx_error_counter` and the counters `rx_overruns` and `tx_failures`. |

Test hooks (mock only; not part of the backend contract and not reachable through the facade except via `bus.backend()`):

| API | Description |
|---|---|
| `inject(frame)` | Plays the bus delivering `frame` to this node. `ok` when buffered for `receive()`; `not_found` when no installed filter matches (the frame is dropped silently, nothing is counted); `full` when the receive queue overflowed (the frame is dropped and `rx_overruns` increments); `not_configured` when closed. The operating mode is not consulted: frames can be injected in every mode. |
| `pop_sent(out)` | Takes the oldest frame the node transmitted: `ok` or `empty`. Works whether or not the node is open. |
| `sent_count()` | Number of transmitted frames waiting for `pop_sent()`. |
| `fail_next_send(status)` | Makes the next `send()` that reaches the fault check return `status` (any value, not only error codes). `castle::status::ok` disarms. The fault is consumed by that call and increments `tx_failures`. |
| `set_bus_state(state, tx_errors = 0, rx_errors = 0)` | Scripts the state and error counters returned by `query_status()`. Only `bus_state::bus_off` changes behaviour (transmission fails); the other states are reported only. Can be called while closed, but `open()` resets the values. |
| `configuration()` | The `can_config` of the last successful `open()` (default `can_config()` before the first one). |
| `filter_count()` | Number of installed filters. |

## Usage Example
```cpp
#include "castle_ext/io/can/backends/mock.hpp"
#include "castle_ext/io/can/bus.hpp"
#include <assert.h>
#include <errno.h>

namespace can = castle::io::can;

int main()
{
    can::basic_bus<can::mock::backend> bus;
    assert(bus.open("vcan0", can::can_config(can::bitrate_500k)) == castle::status::ok);

    // Nothing is received before a filter is installed, exactly like the hardware backends.
    can::frame incoming(0x123U);
    assert(incoming.set_data({0x01U, 0x02U}) == castle::status::ok);
    assert(bus.backend().inject(incoming) == castle::status::not_found);

    assert(bus.add_filter(can::filter::exact(0x123U)) == castle::status::ok);
    assert(bus.backend().inject(incoming) == castle::status::ok);

    can::frame received;
    assert(bus.receive(received) == castle::status::ok);
    assert(received == incoming);
    assert(bus.receive(received) == castle::status::empty);

    // Transmitted frames are read back by the test.
    const uint8_t payload[2] = {0xAAU, 0xBBU};
    assert(bus.send(0x321U, payload, 2U) == castle::status::ok);
    can::frame sent;
    assert(bus.backend().pop_sent(sent) == castle::status::ok);
    assert(sent.id == 0x321U && sent.data_length() == 2U && sent.data[1] == 0xBBU);

    // Script faults.
    bus.backend().fail_next_send(castle::status::full);
    assert(bus.send(0x321U, payload, 2U) == castle::status::full);
    assert(bus.send(0x321U, payload, 2U) == castle::status::ok);

    bus.backend().set_bus_state(can::bus_state::bus_off, 255U, 0U);
    int system_error = 0;
    assert(bus.send(0x321U, payload, 2U, can::id_format::standard, can::no_wait, &system_error)
           == castle::status::system_call_error);
    assert(system_error == ENETUNREACH);

    // One scripted failure plus one bus-off failure.
    can::bus_status status;
    assert(bus.query_status(status) == castle::status::ok);
    assert(status.state == can::bus_state::bus_off && status.tx_failures == 2U);
}
```
See `samples/castle_ext/io/can/backends/mock.cpp` for a standalone sample.

## Constraints & Notes
- **Never blocks.** `send()` and `receive()` accept a timeout for contract compatibility and ignore it: `receive()` returns `empty` and a full transmit queue returns `full` immediately, even for `wait_forever`. A test that needs a frame must `inject()` it first. The facade still rejects negative timeouts with `invalid_argument` before the backend is called.
- **Not thread safe.** There are no locks; call it from one thread, or serialize externally. Use it from a single test thread and inject frames synchronously instead of from a helper thread.
- **No hidden allocation.** All state (filter table, both ring buffers, counters) is inline in the object, so the size is fixed by the three template parameters.
- **Same receive semantics as hardware.** After `open()` no filter is installed. `inject()` accepts a frame only when at least one installed filter matches (`frame.format == filter.format` and `((frame.id ^ filter.id) & filter.mask) == 0`; data and remote frames are not distinguished). A frame matching several filters is queued once, unlike the Zephyr backend, which delivers it once per matching filter. `bus.accept_all()` uses two filter slots (standard and extended), so `MaxFilters` below 2 cannot support it.
- **Loopback.** In `can_mode::loopback`, `send()` goes through the same filter check as `inject()`: a frame is received only if a filter matches, and a frame that no filter matches or that overflows the receive queue is still reported as sent (`ok`); the overflow increments `rx_overruns`. Looped-back frames never reach the transmit queue, so `pop_sent()` returns `empty` and `sent_count()` stays zero.
- **Listen-only.** In `can_mode::listen_only`, `send()` returns `invalid_config`; receiving through `inject()` keeps working. The mock accepts this mode, unlike the [SocketCAN backend](socket_can.md), which rejects it.
- **Fault scripting.** `fail_next_send()` is a single slot: arming it twice before a `send()` keeps only the last status. It is cleared by `open()`. Scripted bus-off blocks `send()` until `set_bus_state()` is called with another state; it does not alter the receive path.
- **Counters.** `tx_failures` counts only scripted failures (`fail_next_send()` and bus-off), `rx_overruns` counts frames dropped because the receive queue was full. Both reset on `open()`. The error counters shown by `query_status()` are whatever `set_bus_state()` last stored.
- **Queue ordering.** Both queues are FIFO. The transmit queue holds the frames until the test pops them; unread frames beyond `TxQueueDepth` make `send()` return `full` (no frame is lost, the new one is rejected).
- **Unit-testing pattern.** Wrap the code under test around a `basic_bus<mock::backend>` (or take the bus as a template parameter), open it, install the filters the code under test installs, then alternate `backend().inject(...)` / `receive()` and `send()` / `backend().pop_sent(...)`. Use `mock::basic_backend<2U, 2U, 2U>` to exercise the `full` and overrun paths cheaply. Assert on `configuration()` to verify the bit rate and mode the code requested, and on `filter_count()` to verify filter bookkeeping.
- The mock models the portable contract, not a specific controller. Behaviour that depends on real hardware or the kernel (acknowledgement, arbitration, error frames, interface naming) is not modelled; verify it against the [SocketCAN](socket_can.md) and [Zephyr](zephyr.md) backends.
