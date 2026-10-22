# CAN SocketCAN Backend

## Overview
`socket_can::basic_backend<MaxFilters>` drives a Linux CAN interface through a raw `PF_CAN` / `CAN_RAW` socket. It creates the socket, resolves the interface with `SIOCGIFINDEX`, binds a `struct sockaddr_can`, switches the descriptor to non-blocking, close-on-exec mode and then works directly on the descriptor with `read`, `write` and `poll` through CASTLE's [`fd`](../../../io/fd/fd.md) helpers. Frames are exchanged as classic `struct can_frame`; the kernel's error frames are consumed inside `receive()` and decoded into the state and counters returned by `query_status()`. It implements the backend contract of [`basic_bus`](../bus.md), so application code is identical to the [mock](mock.md) and [Zephyr](zephyr.md) builds.

An MCP2515 on SPI is an ordinary interface (`can0`) to this backend once the kernel's `mcp251x` driver is bound through the device tree. Bit timing belongs to the interface, not to the application.

## Header
`#include "castle_ext/comm/can/backends/socket_can.hpp"`

## Dependencies
- [`../bus.md`](../bus.md)
- [`../frame.md`](../frame.md)
- [`../filter.md`](../filter.md)
- [`../types.md`](../types.md)
- [`../../../io/fd/descriptor.md`](../../../io/fd/descriptor.md)
- [`../../../io/fd/control.md`](../../../io/fd/control.md)
- [`../../../io/fd/poll.md`](../../../io/fd/poll.md)
- [`../../../../chrono/clocks.md`](../../../../chrono/clocks.md)
- [`../../../../error/status.md`](../../../../error/status.md)

## Public API
`socket_can::backend` is the alias for `basic_backend<>` (eight filters). `socket_can_available` is `true` when the Linux SocketCAN headers were found at build time (`CASTLE_USING_IO_CAN_VIA_SOCKET_CAN`). `endpoint_type` is `const char*`: an interface name (`"can0"`, `"vcan0"`, at most 15 characters) or the literal `"any"`.

| API | Description |
|---|---|
| `open(endpoint, config, system_error)` | Creates and prepares the socket, see the call sequence below. `ok`; `invalid_argument` for a null, empty or over-long (16 characters or more) name; `invalid_config` when already open or for `can_mode::listen_only`; `system_call_error` with `errno` in `system_error` (for example `ENODEV` for an unknown interface); `not_configured` when SocketCAN is unavailable. On any failure the new descriptor is closed and the backend stays closed. |
| `adopt(descriptor, system_error = nullptr)` | Takes ownership of an already created CAN socket (also useful for tests). `ok`; `invalid_argument` for a negative descriptor; `invalid_config` when already open; or the `fcntl()` failure. Ownership transfers even on failure, so the descriptor is closed when adoption is refused. Only close-on-exec and non-blocking are applied; filters, error mask and `CAN_RAW_RECV_OWN_MSGS` stay as the caller set them. Starts with an empty filter table. |
| `close(system_error)` | Forgets all filters, enters `bus_state::stopped` and closes the descriptor. `not_configured` when already closed. |
| `is_open()`, `native_handle()` | Open state and the socket descriptor (`-1` when closed), for platform-specific `setsockopt()` such as `CAN_RAW_LOOPBACK` or `CAN_RAW_JOIN_FILTERS`. |
| `send(frame, timeout, system_error)` | One non-blocking `write()` of a `struct can_frame`. `ok`; `full` when the socket buffer (`EAGAIN`) or the interface queue (`ENOBUFS`) stayed full for the whole timeout; `data_loss` for a short write; `system_call_error` (for example `ENETDOWN`, `ENXIO`); `not_configured`. |
| `receive(out, timeout, system_error)` | Reads the next data or remote frame, consuming error frames on the way. `ok`; `empty` when nothing arrived within the timeout; `data_loss` for a short read, or a frame whose DLC exceeds 8; `system_call_error` (for example `ENETDOWN`); `not_configured`. |
| `add_filter(filter, handle, system_error)` | Installs a filter and re-applies the whole list with `CAN_RAW_FILTER`. `ok`; `full` when `MaxFilters` filters are installed; `system_call_error` when the kernel rejects the list (the filter is not kept); `not_configured`. |
| `remove_filter(handle, system_error)` | Removes a filter and re-applies the list. `ok`; `not_found` for a handle that is not installed; `system_call_error` when the kernel rejects the list (the filter is restored in the table); `not_configured`. |
| `clear_filters(system_error)` | Empties the list and applies it, so the socket receives no data frames. |
| `query_status(out, system_error)` | `ok` with the state and counters learned from error frames; `not_configured` when closed. |
| `native::error_mask` | Error classes requested with `CAN_RAW_ERR_FILTER`: `CAN_ERR_TX_TIMEOUT | CAN_ERR_CRTL | CAN_ERR_BUSOFF | CAN_ERR_RESTARTED`. |
| `native::to_kernel(frame, can_frame&)` | Fills a kernel frame: identifier masked to 11 or 29 bits, `CAN_EFF_FLAG` for extended, `CAN_RTR_FLAG` for remote, DLC and the zero-padded 8-byte payload. |
| `native::from_kernel(can_frame, frame&)` | `ok`, or `data_loss` for a frame with `CAN_ERR_FLAG` or a DLC above 8 (`out` untouched). |
| `native::to_kernel_filter(filter)` | `can_id = id | CAN_EFF_FLAG` (extended only), `can_mask = mask | CAN_EFF_FLAG`; the format flag is always part of the mask, so standard and extended frames never match each other's filters. |
| `native::apply_error_frame(can_frame, bus_status&)` | Folds an error frame into `status`, see the decoding table. |

The `native` namespace is declared only when `socket_can_available` is `true`.

### Kernel call sequence of `open()`
| Step | Call | Failure |
|---|---|---|
| 1 | `socket(PF_CAN, SOCK_RAW, CAN_RAW)` | `system_call_error` |
| 2 | `fcntl(F_SETFD, FD_CLOEXEC)` and `fcntl(F_SETFL, O_NONBLOCK)` through `fd::basic_fd` | the `fcntl()` status |
| 3 | `ioctl(SIOCGIFINDEX)` for the name (skipped for `"any"`, which uses index 0), then `bind()` of `struct sockaddr_can { AF_CAN, ifindex }` | `system_call_error` (`ENODEV` for an unknown interface) |
| 4 | `setsockopt(CAN_RAW_FILTER, NULL, 0)`: an empty list, so no data frames are received yet | `system_call_error` |
| 5 | `setsockopt(CAN_RAW_ERR_FILTER, error_mask)` | `system_call_error` |
| 6 | `setsockopt(CAN_RAW_RECV_OWN_MSGS, 1)` for `can_mode::loopback`, otherwise `0` | `system_call_error` |

### Error-frame decoding
`receive()` reads every kernel frame; frames with `CAN_ERR_FLAG` are never returned to the caller but passed to `native::apply_error_frame()`. Only the classes in `native::error_mask` are requested; protocol violations, bus errors, lost arbitration and acknowledgement errors are never delivered. Within one error frame the rows are applied in this order, so a later row overrides an earlier `state`:

| Kernel error bit | Effect on `bus_status` |
|---|---|
| `CAN_ERR_TX_TIMEOUT` (in `can_id`) | `tx_failures` +1 |
| `CAN_ERR_RESTARTED` | `state = error_active` |
| `CAN_ERR_BUSOFF` | `state = bus_off` |
| `CAN_ERR_CRTL` with `data[1] & CAN_ERR_CRTL_RX_OVERFLOW` | `rx_overruns` +1 |
| `CAN_ERR_CRTL` with `data[1] & CAN_ERR_CRTL_TX_OVERFLOW` | `tx_failures` +1 |
| `CAN_ERR_CRTL` with `CAN_ERR_CRTL_RX_PASSIVE` or `CAN_ERR_CRTL_TX_PASSIVE` | `state = error_passive` |
| `CAN_ERR_CRTL` with only `CAN_ERR_CRTL_RX_WARNING` or `CAN_ERR_CRTL_TX_WARNING` | `state = error_warning` |
| `CAN_ERR_CRTL` with only `CAN_ERR_CRTL_ACTIVE` | `state = error_active` |
| `CAN_ERR_CRTL` and the kernel flags the counters valid (`CAN_ERR_CNT` in `can_id`; always assumed when the kernel headers lack that flag) | `tx_error_counter = data[6]`, `rx_error_counter = data[7]` |

A fresh session starts with `state = error_active`, zero counters and zero drop counts. A `CRTL` frame that sets none of the state bits leaves `state` unchanged.

## Usage Example
Bring the interface up first; the application never sets the bit rate (the `bitrate` of `can_config` is not applied on Linux):
```sh
# MCP2515 bound through the mcp251x driver (Raspberry Pi: dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=25)
sudo ip link set can0 up type can bitrate 500000 restart-ms 100
ip -details -statistics link show can0

# No hardware: a virtual interface
sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0
```
Application code, normally through `castle::comm::can::bus<socket_can::backend>` or `default_bus`:
```cpp
castle::comm::can::bus<castle::comm::can::socket_can::backend> bus;
int system_error = 0;
if (bus.open("can0", castle::comm::can::can_config(castle::comm::can::bitrate_500k), &system_error)
        != castle::status::ok)
{
    return; // for example system_error == ENODEV: the interface does not exist
}
(void)bus.accept_all();        // without a filter nothing is received
```
The same code can be tested without any CAN interface by adopting one end of a datagram `socketpair`, which carries 16-byte `struct can_frame` records exactly like a `CAN_RAW` socket:
```cpp
#include "castle_ext/comm/can/backends/socket_can.hpp"
#include "castle_ext/comm/can/bus.hpp"
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/can.h>

namespace can = castle::comm::can;

int main()
{
    int pair[2] = {-1, -1};
    assert(::socketpair(AF_UNIX, SOCK_DGRAM, 0, pair) == 0);

    can::basic_bus<can::socket_can::backend> bus;
    assert(bus.backend().adopt(pair[0]) == castle::status::ok);

    // The peer puts a standard data frame on the "wire".
    struct can_frame wire = can_frame();
    wire.can_id = 0x123U;
    wire.can_dlc = 2U;
    wire.data[1] = 0x22U;
    assert(::write(pair[1], &wire, sizeof(wire)) == static_cast<ssize_t>(sizeof(wire)));

    can::frame received;
    assert(bus.receive(received) == castle::status::ok);
    assert(received.id == 0x123U && received.data_length() == 2U && received.data[1] == 0x22U);
    assert(bus.receive(received) == castle::status::empty);

    // send() writes one struct can_frame.
    const uint8_t payload[3] = {1U, 2U, 3U};
    assert(bus.send(0x1ABCDEF0U, payload, 3U, can::id_format::extended) == castle::status::ok);
    struct can_frame sent = can_frame();
    assert(::read(pair[1], &sent, sizeof(sent)) == static_cast<ssize_t>(sizeof(sent)));
    assert((sent.can_id & CAN_EFF_FLAG) != 0U && (sent.can_id & CAN_EFF_MASK) == 0x1ABCDEF0U);

    // A socketpair is not a CAN_RAW socket: the kernel refuses the filter list and the table rolls back.
    int system_error = 0;
    assert(bus.add_filter(can::filter::exact(0x123U), nullptr, &system_error)
           == castle::status::system_call_error);
    assert(system_error != 0);

    assert(bus.close() == castle::status::ok);
    (void)::close(pair[1]);
}
```
See `samples/castle_ext/comm/can/backends/socket_can.cpp` for a standalone sample.

## Constraints & Notes
- **No filter means no reception.** `open()` and `adopt()` start with an empty tracked table, and `open()` installs an empty kernel list (`CAN_RAW_FILTER` with length zero), so no data frames arrive until `add_filter()` or `accept_all()` (two filters, standard and extended) is called. `clear_filters()` and removing the last filter return to that state. Error frames are selected separately by `CAN_RAW_ERR_FILTER`. A frame matching several filters is delivered once by the kernel, unlike the [Zephyr backend](zephyr.md). An adopted socket keeps any filters the caller installed on the descriptor itself; the tracked table does not know them.
- **Filter matching.** Filters use the portable rule `((frame.id ^ filter.id) & filter.mask) == 0` plus an equal identifier format. Remote and data frames are not distinguished. Each change rebuilds the whole `can_filter` list on the stack (`MaxFilters` entries of 8 bytes) with one `setsockopt()`; the kernel limits the list to `CAN_RAW_FILTER_MAX` entries.
- **Timeouts.** `send()` makes one immediate attempt. When it finds `EAGAIN` or `ENOBUFS` and the timeout is positive, it waits for writability with [`fd::wait_fd()`](../../../io/fd/poll.md) for at most the timeout (`wait_forever` waits indefinitely), then retries exactly once. If that retry or the wait fails with `time_out`, the result is `full`. `ENOBUFS` (the interface transmit queue is full) is reported as `full` the same way; the wait watches the socket's writability, so the timeout is designed for `EAGAIN` back-pressure and a persistent `ENOBUFS` still ends as `full` after the single retry. `no_wait` makes one attempt. `receive()` loops over `read()`: error frames are absorbed and reading continues; an empty read waits with `wait_fd()` until a deadline computed with `steady_clock` (rounded up to whole milliseconds), then returns `empty`. `no_wait` returns `empty` right away. A timeout above `INT_MAX` ms is clamped for `poll()`.
- **`system_error`.** Cleared on entry. `receive()` returning `empty` always leaves `0` (a non-blocking `EAGAIN` is not an error). A `send()` returning `full` reports the `errno` that caused it (`EAGAIN` or `ENOBUFS`), except after a wait that timed out, where it is `0`.
- **Bit timing belongs to the interface.** `can_config::bitrate` is ignored on Linux (a value above `max_bitrate` is still rejected by the facade). Set it with `ip link set can0 up type can bitrate 500000` (with optional `sample-point`, `restart-ms`). A stopped interface makes I/O fail (for example with `ENETDOWN`), reported as `system_call_error`. Automatic bus-off recovery requires `restart-ms` (otherwise `ip link set can0 type can restart`), after which the kernel emits `CAN_ERR_RESTARTED`.
- **`listen_only` is rejected** with `invalid_config` (before any socket is created). Listen-only is a controller property of the interface (`ip link set can0 type can bitrate 500000 listen-only on`) that cannot be selected through a `CAN_RAW` socket. Configure it on the interface and open with `can_mode::normal`.
- **Loopback.** `can_mode::loopback` enables `CAN_RAW_RECV_OWN_MSGS`: the socket receives its own transmitted frames, subject to its filters. It does not put the controller into loopback mode. On a real interface the frame is echoed only after the controller transmitted it, so a missing acknowledging node leaves it pending; for bench tests use `vcan` or `ip link set can0 type can bitrate 500000 loopback on`.
- **`"any"` binds receive-only.** Binding to ifindex 0 receives from every CAN interface; `send()` on such a socket fails with `system_call_error` (`ENXIO`) because no interface is specified.
- **Status is lazy.** `query_status()` returns what `receive()` has consumed so far. Nothing refreshes it from the kernel, so call `receive()` regularly (even with `no_wait`) when the state matters. The count of frames dropped in the socket receive buffer is not observed by this backend; only the controller-reported `CAN_ERR_CRTL_RX_OVERFLOW` increments `rx_overruns`. For an adopted socket, error frames appear only if the caller set `CAN_RAW_ERR_FILTER` itself.
- **Frame conversion.** `from_kernel()` returns `data_loss` for error frames and a DLC above 8; `receive()` also returns `data_loss` for a read that is not exactly one `struct can_frame`. The offending frame is consumed. CAN FD is out of scope (`CAN_RAW_FD_FRAMES` is never enabled).
- **Thread safety.** One thread may call `send()` while another calls `receive()`. Concurrent calls of the same kind, filter changes during I/O, and `query_status()` concurrent with `receive()` (which updates the state) need external synchronization. The backend owns the descriptor and closes it on destruction; do not close `native_handle()` yourself.
- **Stub behaviour.** Without SocketCAN (a non-Linux target, no POSIX APIs, a Zephyr build or missing `<linux/can.h>`), `socket_can_available` is `false`, the `native` namespace does not exist, `open()` returns `invalid_argument` for a null or empty name and `not_configured` otherwise, and every other call returns `not_configured`. `is_open()` is `false` and `native_handle()` is `-1`. The class stays instantiable so portable code builds everywhere.
- **Unit testing without a CAN interface.** No `vcan` or root is needed for most behaviour: `socket(PF_CAN, SOCK_RAW, CAN_RAW)` works unprivileged, binding to `"any"` works, and `adopt()` accepts one end of an `AF_UNIX` / `SOCK_DGRAM` `socketpair`, which carries `struct can_frame` datagrams for `read`, `write` and `poll`. On such a socketpair `setsockopt(SOL_CAN_RAW, ...)` fails (observed `EOPNOTSUPP` on Linux 5.15), which exercises the `add_filter()` / `remove_filter()` / `clear_filters()` failure and rollback paths. The `native::` translation functions are pure, so frame, filter and error-frame decoding are verified directly on `struct can_frame` values. Opening a named interface in a sandbox without CAN fails with `ENODEV`.
- Prefer the [mock backend](mock.md) for application logic tests and this backend for integration tests on `vcan0` or real hardware.
