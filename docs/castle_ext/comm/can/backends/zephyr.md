# CAN Zephyr Backend

## Overview
`zephyr::basic_backend<RxQueueDepth, MaxFilters>` adapts Zephyr's `can_*()` driver API (`const struct device*` plus `can_send()`, `can_add_rx_filter()`, ...) to the backend contract of [`basic_bus`](../bus.md). It targets controllers such as the Microchip MCP2515 on SPI. Receiving never allocates and creates no thread: the driver's receive callback converts each frame, pushes it into a fixed single-producer / single-consumer queue and gives a `k_sem`, and `receive()` takes the semaphore (honouring the timeout) and pops the queue. Sending hands the frame to a hardware transmit mailbox and returns without waiting for a bus acknowledgement.

For the platform-neutral bus API and the Linux counterpart see [`can.md`](../can.md) and [`socket_can.md`](socket_can.md).

## Header
`#include "castle_ext/comm/can/backends/zephyr.hpp"`

## Dependencies
- [`../bus.md`](../bus.md)
- [`../frame.md`](../frame.md)
- [`../filter.md`](../filter.md)
- [`../types.md`](../types.md)
- [`../../../../container/mirrored_ring_buffer.md`](../../../../container/mirrored_ring_buffer.md)
- [`../../../../sync/atomic.md`](../../../../sync/atomic.md)
- [`../../../../error/status.md`](../../../../error/status.md)
- [`../../../../core/config.md`](../../../../core/config.md)
- Zephyr 3.2 or newer: `<zephyr/device.h>`, `<zephyr/drivers/can.h>`, `<zephyr/kernel.h>` when `CASTLE_USING_IO_CAN_VIA_ZEPHYR` is `1`.

## Public API
`zephyr::backend` is the alias for `basic_backend<16U, 8U>`. `zephyr_available` is `true` when the Zephyr CAN API is available in this build. `endpoint_type` is `const struct device*`, typically `DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))`.

| Template parameter | Default | Meaning |
|---|---|---|
| `RxQueueDepth` | `16U` | Frames buffered between the driver callback and `receive()`. |
| `MaxFilters` | `8U` | Simultaneously installed filters; keep within the controller's limit. |

| API | Description |
|---|---|
| `open(device, config, system_error)` | `can_stop()` (an already-stopped controller, `-EALREADY`, is fine), `can_set_mode()`, `can_set_bitrate()` only when `config.bitrate != 0` (zero keeps the devicetree `bus-speed`), `can_start()`. `ok`; `invalid_argument` for a null device; `invalid_config` when already open or for a driver `-ENOTSUP` / `-ERANGE`; `not_configured` when `device_is_ready()` is false or Zephyr CAN is unavailable; otherwise the mapped driver error. On a driver failure the controller is left stopped and the backend stays closed. |
| `close(system_error)` | Removes every filter, stops the controller (`-EALREADY` is fine), releases the device and discards queued frames. `not_configured` when already closed. The backend is closed even if `can_stop()` fails (the mapped error is returned). |
| `is_open()` | `true` while a device is held. |
| `send(frame, timeout, system_error)` | `can_send()` with the converted frame, the converted timeout and a completion callback. `ok` once a mailbox accepted the frame; `full` (`-EAGAIN`, no mailbox within the timeout); `not_configured` (`-ENETDOWN`, controller stopped, or backend closed); `system_call_error` otherwise, for example `ENETUNREACH` while bus-off. |
| `receive(out, timeout, system_error)` | `k_sem_take()` with the converted timeout, then pops one frame. `ok`; `empty` when the semaphore could not be taken in time; `not_configured`. |
| `add_filter(filter, handle, system_error)` | `can_add_rx_filter()` with the static callback. `ok`; `full` when `MaxFilters` filters are installed (checked first, no driver call) or the driver reports `-ENOSPC`; `invalid_config` when the controller cannot express the filter (`-ENOTSUP`); the mapped driver error otherwise; `not_configured`. The returned `handle` is the backend's own slot index, not the driver's filter id. |
| `remove_filter(handle, system_error)` | `can_remove_rx_filter()`. `ok`; `not_found` for a handle that is not installed; `not_configured`. |
| `clear_filters(system_error)` | Unregisters every filter. `ok` or `not_configured`. |
| `query_status(out, system_error)` | `can_get_state()` for `state` and the hardware error counters, plus the software counters `rx_overruns` and `tx_failures`. `not_configured` when closed, or the mapped driver error (with `out` left default). |
| `native::to_status(result)` | Always available. Maps a driver return value to a `castle::status`, see the table below. |
| `native::to_zephyr(frame, can_frame&)` | Fills a driver frame: `id`, `dlc`, flags `CAN_FRAME_IDE` for extended and `CAN_FRAME_RTR` for remote, and `data_length()` payload bytes. |
| `native::from_zephyr(can_frame, frame&)` | `ok`, or `data_loss` for a CAN FD frame (`CAN_FRAME_FDF`) or a DLC above 8. |
| `native::to_zephyr_filter(filter)` | `{id, mask, flags}` with `CAN_FILTER_IDE` for extended filters. |
| `native::to_zephyr_mode(mode)` | `normal` to `CAN_MODE_NORMAL`, `loopback` to `CAN_MODE_LOOPBACK`, `listen_only` to `CAN_MODE_LISTENONLY`. |
| `native::to_bus_state(state)` | `CAN_STATE_ERROR_ACTIVE`, `_WARNING`, `_PASSIVE`, `_BUS_OFF` to the matching `bus_state`; anything else (including `CAN_STATE_STOPPED`) to `stopped`. |
| `native::to_kernel_timeout(timeout)` | `wait_forever` to `K_FOREVER`; zero to `K_NO_WAIT`; otherwise `K_MSEC()` clamped to `INT32_MAX` ms. |

All `native::` functions except `to_status()` are declared only when `zephyr_available` is `true`.

### Driver errors (`native::to_status`)
`to_status()` takes the driver return value (`0` or a negative errno). Every failing call stores the positive errno (`-result`) in `system_error`.

| Driver result | Castle status |
|---|---|
| `>= 0` (including a filter id) | `ok` |
| `-EAGAIN`, `-ENOSPC` | `full` |
| `-EINVAL` | `invalid_argument` |
| `-ENOTSUP`, `-ERANGE` | `invalid_config` |
| `-ENETDOWN` | `not_configured` |
| anything else (`-ENETUNREACH`, `-EIO`, `-EBUSY`, ...) | `system_call_error` |

## Usage Example
Enable the driver in `prj.conf` and describe the controller in the board overlay (pins, SPI controller and oscillator are an example; adapt them to your board):
```
CONFIG_CPP=y
CONFIG_STD_CPP17=y
CONFIG_GPIO=y
CONFIG_SPI=y
CONFIG_CAN=y
CONFIG_CAN_MCP2515=y
CONFIG_CAN_MAX_FILTER=5
```
```dts
&spi1 {
    status = "okay";
    cs-gpios = <&gpio0 4 GPIO_ACTIVE_LOW>;

    mcp2515: can@0 {
        compatible = "microchip,mcp2515";
        reg = <0>;
        spi-max-frequency = <1000000>;
        int-gpios = <&gpio0 5 GPIO_ACTIVE_LOW>;
        osc-freq = <16000000>;
        bus-speed = <500000>;
        status = "okay";
    };
};

/ {
    chosen {
        zephyr,canbus = &mcp2515;
    };
};
```
```cpp
#include "castle_ext/comm/can/can.hpp"
#include <zephyr/device.h>

namespace can = castle::comm::can;

// Static storage: the driver callbacks hold a pointer to the backend object.
static can::bus<can::zephyr::backend> bus;

void run()
{
    const struct device* const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

    int system_error = 0;
    if (bus.open(can_dev, can::can_config(can::bitrate_500k), &system_error) != castle::status::ok)
    {
        return; // not_configured: the device is not ready; or a mapped driver error
    }
    (void)bus.accept_all();                         // two filters; nothing is received without one

    const uint8_t payload[2] = {0x01U, 0x02U};
    (void)bus.send(0x123U, payload, 2U, can::id_format::standard, castle::chrono::milliseconds(10));

    can::frame rx;
    if (bus.receive(rx, castle::chrono::milliseconds(100)) == castle::status::ok)
    {
        // use rx.id, rx.payload()
    }
}
```
See `samples/castle_ext/comm/can/backends/zephyr.cpp` for a standalone sample (it builds on the host, where the backend is a stub).

## Constraints & Notes
- **Version.** Zephyr 3.2 or newer: the `<zephyr/...>` include prefix, `struct can_frame` and `can_start()` / `can_stop()` are required. The host test double mirrors the Zephyr 3.7 signatures. Other Kconfig: `CONFIG_CAN_INIT_PRIORITY` is not needed (the application calls `open()` at run time after `device_is_ready()`), and the controller's interrupt thread stack option (`CONFIG_CAN_MCP2515_INT_THREAD_STACK_SIZE` in current releases) only has to fit the receive callback, which converts one frame and pushes it into the queue. For the complete bring-up of an MCP2515 on Zephyr and on Linux see [`can.md`](../can.md).
- **Bit rate and mode.** `can_config::bitrate` is applied with `can_set_bitrate()`; zero keeps the devicetree `bus-speed`. `can_mode::listen_only` and `loopback` are applied with `can_set_mode()` (a driver that does not support a mode answers `-ENOTSUP`, giving `invalid_config`). The controller is restarted by every `open()` and left stopped by `close()`.
- **Receive path.** `can_add_rx_filter()` registers one static callback per filter with the backend as user data. The callback converts the frame with `native::from_zephyr()` and pushes it into `container::mirrored_ring_buffer` (SPSC); only a successful push gives the `k_sem`. Frames that fail conversion (CAN FD, DLC above 8) are dropped silently. When the queue is full the frame is dropped and `bus_status::rx_overruns` increments. `receive()` returns `empty` when `k_sem_take()` fails, which is a timeout, or `-EBUSY` for `no_wait`.
- **Send path.** `send()` passes the frame and a completion callback to `can_send()`. `ok` means a transmit mailbox accepted the frame, not that the bus acknowledged it; completions with a non-zero error increment `bus_status::tx_failures` (an atomic counter). With a non-zero timeout `can_send()` blocks for a free mailbox, so do not call it from an interrupt handler (Zephyr rule); `no_wait` is the interrupt-safe form. The frame is copied by the driver call, so `send()` does not retain the caller's data.
- **Threading.** The receive callback is the single producer. The MCP2515 driver runs it from its interrupt thread; other drivers may call it from interrupt context, which is why no locks are taken. Use exactly one consumer thread for `receive()`. `send()` is as thread safe as the driver's `can_send()`. `open()`, `close()` and the filter calls are not synchronized with each other or with `receive()`: issue them from one control context. `rx_overruns` and `tx_failures` are `castle::atomic<uint32_t>` so `query_status()` can read them from any thread.
- **Object lifetime.** The registered callbacks hold a pointer to the backend, so it is neither copyable nor movable (and so neither is `basic_bus<zephyr::backend>`); give it static storage or keep it alive until after `close()`. The destructor calls `close()`, which unregisters the callbacks and stops the controller.
- **Overlapping filters deliver duplicates.** Each filter has its own registration, so a frame that matches two filters (for example `any(standard)` plus `exact(0x123)`) is queued once per matching filter and `receive()` returns it twice. The [mock](mock.md) and [SocketCAN](socket_can.md) backends deliver it once. `accept_all()` installs `any(standard)` and `any(extended)`, which cannot overlap because the identifier format is part of the match.
- **Filter limits.** `add_filter()` returns `full` as soon as `MaxFilters` is reached, otherwise whatever the driver says. `CONFIG_CAN_MAX_FILTER` (default 5) is the driver-side limit, and the controller has hardware limits of its own (`can_get_max_filters()`), so set `MaxFilters` no higher than both and remember that `accept_all()` consumes two. Filters on the MCP2515 share acceptance masks, so a combination can be refused below the limit: `-ENOSPC` gives `full` and `-ENOTSUP` gives `invalid_config`. A `MaxFilters` larger than the driver limit only costs a few bytes in the slot table.
- **Queue depth.** `RxQueueDepth` frames must cover the longest time the consumer thread can stay away from `receive()`. A classic frame with 8 data bytes takes at most about 130 bits, 260 microseconds at 500 kbit/s, so a consumer that may be late by 10 ms needs about 39 slots; add margin for bursts. Each slot stores one `frame` (roughly 16 bytes), so the cost of generous depth is small; `rx_overruns` in `query_status()` shows whether it was enough. The queue is cleared and the semaphore reset by `open()` and `close()`.
- **Bus state.** `query_status()` reads the controller state on demand: `bus_off` and `error_passive` are visible there, and `send()` fails with `ENETUNREACH` (`system_call_error`) while bus-off. Bus-off recovery is configured in Zephyr itself (the auto-recovery Kconfig option of your release, or `can_recover()`); this backend neither calls `can_recover()` nor registers a state-change callback.
- **Stub behaviour.** Without Zephyr (`CASTLE_USING_IO_CAN_VIA_ZEPHYR == 0`; any non-Zephyr host build) `zephyr_available` is `false`, `struct device` is only forward-declared and the class remains instantiable: `open()` returns `invalid_argument` for a null device and `not_configured` otherwise, `is_open()` is `false`, and every other call returns `not_configured`. Only `native::to_status()` stays available, so the error mapping can be tested everywhere.
- **Host testing with the fake SDK.** `test/castle_ext/io/can/fake_zephyr/zephyr/{device.h,kernel.h,drivers/can.h}` replaces the Zephyr headers (Zephyr 3.7 signatures) so the real backend code runs on a PC. `fake_can` is a scriptable controller: `*_result` members make `can_stop`, `can_start`, `can_set_mode`, `can_set_bitrate`, `can_send`, `can_add_rx_filter` and `can_get_state` fail with a chosen errno, its members record the mode, bit rate, last sent frame and timeout, and `fake_can.deliver(device, frame)` fires the callback of every matching filter like the MCP2515 driver (it has four filter slots, then `-ENOSPC`). The fake semaphore never blocks: taking an empty one fails at once, so tests check behaviour rather than elapsed time. A test translation unit must define `CASTLE_USING_IO_CAN_VIA_ZEPHYR` as `1` before any CASTLE include and add `-Itest/castle_ext/io/can/fake_zephyr` to the include path (`test/castle_ext/io/can/backends/zephyr.cpp`). One-definition rule: that translation unit instantiates the default `zephyr::backend` (`<16U, 8U>`) with the real implementation, so the stub translation unit (`test/castle_ext/io/can/backends/zephyr_stub.cpp`, macro `0`) must instantiate only different template arguments, never `zephyr::backend`.
