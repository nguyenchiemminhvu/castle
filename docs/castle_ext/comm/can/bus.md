# CAN Bus

## Overview
`basic_bus<Backend>` is the backend-independent CAN facade. It owns exactly one backend object and forwards every call to it by direct, inlinable member calls: no virtual functions, no RTTI, no heap. The facade adds what every backend would otherwise repeat: argument validation (`validate_config`, `validate_frame`, `validate_filter`, non-negative timeouts), the optional `system_error` out-parameter convention of Castle's `io` components, and the convenience calls `send(id, bytes, length, ...)` and `accept_all()`.

Backends are compile-time template arguments. `bus<Backend>` is a convenience alias; [`can.md`](can.md) selects `default_bus` for the current platform.

## Header
`#include "castle_ext/comm/can/bus.hpp"`

## Dependencies
- [`../../../core/compiler.md`](../../../core/compiler.md)
- [`../../../core/traits.md`](../../../core/traits.md)
- [`../../../core/types.md`](../../../core/types.md)
- [`../../../error/status.md`](../../../error/status.md)
- [`../../io/fd/control.md`](../../io/fd/control.md) (`fd::detail::clear_system_error`)
- [`frame.md`](frame.md)
- [`filter.md`](filter.md)
- [`types.md`](types.md)

## Public API
All names live in `castle::comm::can`. Every function takes an optional trailing `int* system_error`: it is set to `0` on entry and receives the platform error number (`errno`, or the positive Zephyr driver errno) when a platform call fails. Pass `nullptr` to ignore it.

### Class and aliases
| API | Description |
|---|---|
| `basic_bus<Backend>` | CAN bus facade. A `static_assert` checks the backend contract below at instantiation. |
| `bus<Backend>` | Alias template for `basic_bus<Backend>`. |
| `backend_type`, `endpoint_type` | Member typedefs: `Backend` and `Backend::endpoint_type` (interface name `const char*` on SocketCAN and the mock, `const struct device*` on Zephyr). |
| `basic_bus()` | Defaulted. Constructs the backend closed. |
| copy / move | Copy is deleted; move is defaulted (and therefore unavailable when the backend is not movable, as with the Zephyr backend). |
| `detail::has_bus_backend_contract<Backend>` | Trait behind the `static_assert`: true when `Backend` has `endpoint_type` and the nine member functions below with these argument types. |

### Operations
`castle::status` return values are listed per function. "Backend" statuses depend on the platform; see the backend pages.

| API | Description | Returns |
|---|---|---|
| `open(endpoint_type endpoint, can_config const& config = can_config(), int* system_error = nullptr)` | Validates `config`, then opens, configures and starts the controller. No filter is installed afterwards. | `ok`; `invalid_config` (bad config, controller already open, or mode/bit rate rejected by the platform); `invalid_argument` (null or empty endpoint, over-long interface name); `system_call_error` (platform call failed, e.g. `ENODEV` for an unknown interface); `not_configured` (backend unavailable on this build, or the Zephyr device is not ready). |
| `close(int* system_error = nullptr)` | Removes all filters, stops the controller and releases it. | `ok`; `not_configured` when not open; `system_call_error` if the platform call fails. |
| `is_open() const` | True while a controller is open. `noexcept`, O(1). | `bool` |
| `send(frame const& value, timeout_type timeout = no_wait, int* system_error = nullptr)` | Validates `value` with `validate_frame()` and `timeout.count() >= 0`, then queues the frame. Returns once queued, not once acknowledged. | `ok`; `full` (queue stayed full for the whole timeout); `invalid_argument` (invalid frame or negative timeout); `not_configured` (closed); `invalid_config` (backend refuses transmission, e.g. the mock in `listen_only`); `system_call_error` (e.g. bus-off, interface down); `data_loss` (short write on SocketCAN). |
| `send(uint32_t id, const void* bytes, size_type length, id_format format = id_format::standard, timeout_type timeout = no_wait, int* system_error = nullptr)` | Builds a data frame with `frame::set_data()` and forwards to `send(frame)`. | `out_of_range` (`length > 8`); `invalid_argument` (null `bytes` with non-zero `length`, id too wide for `format`, negative timeout); otherwise as `send(frame)`. |
| `receive(frame& out, timeout_type timeout = no_wait, int* system_error = nullptr)` | Takes the oldest received frame that passed an installed filter. `out` is only written when `ok` is returned. | `ok`; `empty` (nothing arrived within the timeout); `invalid_argument` (negative timeout); `not_configured` (closed); `system_call_error` (e.g. `ENETDOWN`); `data_loss` (SocketCAN: truncated read or a kernel frame with DLC above 8). |
| `add_filter(filter const& value, filter_handle* handle = nullptr, int* system_error = nullptr)` | Validates `value` with `validate_filter()` and installs it. `*handle` is set to `invalid_filter` on entry and to the new handle on success. | `ok`; `full` (no free filter slot); `invalid_argument`; `not_configured` (closed); `invalid_config` (hardware cannot express the filter); `system_call_error` (SocketCAN: kernel rejected the filter list, filter not kept). |
| `accept_all(int* system_error = nullptr)` | Installs `filter::any(standard)` and `filter::any(extended)`: **two** filter slots. If the second fails, the first is removed again. Handles are not returned; undo with `clear_filters()`. | As `add_filter()`. |
| `remove_filter(filter_handle handle, int* system_error = nullptr)` | Removes one filter. | `ok`; `not_found` (handle not installed); `not_configured`; `system_call_error` (SocketCAN: re-applying the list failed, filter kept). |
| `clear_filters(int* system_error = nullptr)` | Removes every filter; `receive()` then yields nothing. | `ok`; `not_configured`; `system_call_error` (SocketCAN: re-applying the list failed). |
| `query_status(bus_status& out, int* system_error = nullptr)` | Reads controller state, error counters and drop counters. `out` is reset to a default `bus_status` first. | `ok`; `not_configured`; `system_call_error` (Zephyr: driver query failed). |
| `backend()` / `backend() const` | Reference to the owned backend for platform access (`native_handle()`/`adopt()` on SocketCAN, `inject()`/`pop_sent()` on the mock). `noexcept`. | `backend_type&` / `backend_type const&` |

`open`, `send`, `receive`, `add_filter`, `accept_all`, `query_status`, `is_open` and `backend` are marked `CASTLE_NODISCARD`: check the returned `castle::status` with `castle::succeeded(...)` before trusting an output parameter.

### Backend contract
```cpp
struct backend {
    using endpoint_type = ...;                    // "can0", const device*, ...

    castle::status open(endpoint_type, can_config const&, int* system_error);
    castle::status close(int* system_error);
    bool           is_open() const;
    castle::status send(frame const&, timeout_type, int* system_error);
    castle::status receive(frame&, timeout_type, int* system_error);
    castle::status add_filter(filter const&, filter_handle&, int* system_error);
    castle::status remove_filter(filter_handle, int* system_error);
    castle::status clear_filters(int* system_error);
    castle::status query_status(bus_status&, int* system_error);
};
```
Backends shipped with Castle: `castle::comm::can::mock::backend` ([`backends/mock.md`](backends/mock.md)), `castle::comm::can::socket_can::backend` ([`backends/socket_can.md`](backends/socket_can.md)) and `castle::comm::can::zephyr::backend` ([`backends/zephyr.md`](backends/zephyr.md)). Shared behaviour is summarised in [`can.md`](can.md).

## Usage Example
See `samples/castle_ext/comm/can/bus.cpp`.

```cpp
#include "castle_ext/comm/can/backends/mock.hpp"
#include "castle_ext/comm/can/bus.hpp"

#include <assert.h>

using namespace castle::comm::can;

int main()
{
    bus<mock::backend> node;
    assert(node.open("can0", can_config(bitrate_500k)) == castle::status::ok);

    // Nothing is received until a filter is installed.
    assert(node.backend().inject(frame(0x123U)) == castle::status::not_found);

    filter_handle handle = invalid_filter;
    assert(node.add_filter(filter::exact(0x123U), &handle) == castle::status::ok);

    frame incoming(0x123U);
    assert(incoming.set_data({0xDEU, 0xADU}) == castle::status::ok);
    assert(node.backend().inject(incoming) == castle::status::ok);   // mock-only hook

    frame received;
    assert(node.receive(received, castle::chrono::milliseconds(100)) == castle::status::ok);
    assert(received == incoming);
    assert(node.receive(received) == castle::status::empty);

    const unsigned char payload[] = {1U, 2U, 3U};
    assert(node.send(0x321U, payload, sizeof(payload)) == castle::status::ok);
    frame sent;
    assert(node.backend().pop_sent(sent) == castle::status::ok);
    assert(sent.data_length() == 3U);

    assert(node.send(frame(0x800U)) == castle::status::invalid_argument);
    assert(node.remove_filter(handle) == castle::status::ok);
    assert(node.remove_filter(handle) == castle::status::not_found);
    assert(node.close() == castle::status::ok);
    assert(node.send(sent) == castle::status::not_configured);
    return 0;
}
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI or virtual dispatch. The facade stores only the backend object, so `sizeof(basic_bus<B>)` is `sizeof(B)`.
- **No locking is added.** Thread safety is the backend's: on SocketCAN one thread may send while another receives, but concurrent calls of the same kind and filter changes during I/O need external synchronisation; Zephyr expects a single consumer thread for `receive()`; the mock is not thread safe.
- The portable semantics hold on every backend: no filter after `open()`; `send()` returns when queued; `full`/`empty` mean "stayed full/empty for the whole timeout"; `no_wait` never blocks; `wait_forever` blocks indefinitely; not open gives `not_configured`; opening twice gives `invalid_config`.
- The facade checks arguments before the backend runs, so validation failures never reach hardware: they return `invalid_config` (configuration) or `invalid_argument` (frame, filter, timeout), and `system_error` is `0` afterwards.
- The `time_out` status is never returned: backends translate poll timeouts to `empty` (receive) or `full` (send).
- A `Backend` without the contract fails one `static_assert` naming the required members instead of deep template errors.
- `accept_all()` consumes two filter slots: a backend configured with `MaxFilters == 1` reports `full` and leaves no filter installed.
- Related: [`frame.md`](frame.md), [`filter.md`](filter.md), [`types.md`](types.md), [`can.md`](can.md), [`detail/slot_table.md`](detail/slot_table.md).
