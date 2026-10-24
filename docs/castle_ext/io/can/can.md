# CAN Umbrella

## Overview
Castle's CAN extension is one classic-CAN (CAN 2.0A/2.0B) API over three interchangeable backends: Linux **SocketCAN**, **Zephyr** CAN and a deterministic in-memory **mock** for host tests. It is built for controllers such as the Microchip **MCP2515** (SPI, classic only): the same application code runs on a Raspberry Pi style Linux board with the kernel `mcp251x` driver and on a Zephyr MCU with the `microchip,mcp2515` driver. The umbrella header collects the platform-neutral frame, filter, configuration and bus facade types plus the mock backend, and selects `default_backend` / `default_bus` at compile time.

### Architecture
```
 application code
        |   frame, filter, can_config, bus_status, castle::status
        v
 +------------------------------+
 | basic_bus<Backend>           |  validation, system_error convention,
 | (bus.hpp, header-only)       |  send(id, bytes), accept_all()
 +------------------------------+
        |   direct inlinable calls (compile-time Backend, no virtual)
        v
 +--------------+---------------------+-----------------+
 | socket_can   | zephyr              | mock            |   backends/
 | ::backend    | ::backend           | ::backend       |
 +--------------+---------------------+-----------------+
        |                 |                   |
 Linux kernel      Zephyr CAN driver     in-memory queues
 PF_CAN/CAN_RAW    can_send(),           (inject(), pop_sent())
 + mcp251x driver  can_add_rx_filter()
        |                 |
        +------ SPI ------+---- MCP2515 --- CAN bus
```

### Compile-time backend selection
`default_backend` is chosen by the preprocessor, using `CASTLE_USING_IO_CAN_VIA_ZEPHYR` and `CASTLE_USING_IO_CAN_VIA_SOCKET_CAN` from [`castle/core/config.hpp`](../../../core/config.md):

| Condition | `default_backend` | `default_backend_is_hardware` |
|---|---|---|
| `CASTLE_USING_IO_CAN_VIA_ZEPHYR` | `zephyr::backend` | `true` |
| `CASTLE_USING_IO_CAN_VIA_SOCKET_CAN` | `socket_can::backend` | `true` |
| neither | `mock::backend` | `false` |

`CASTLE_USING_IO_CAN_VIA_ZEPHYR` is `1` when `__ZEPHYR__` is defined and `<zephyr/drivers/can.h>` exists. `CASTLE_USING_IO_CAN_VIA_SOCKET_CAN` is `1` for Linux POSIX builds (not Zephyr) where `<linux/can.h>` and `<linux/can/raw.h>` exist. Both can be overridden on the command line (`-DCASTLE_USING_IO_CAN_VIA_SOCKET_CAN=0`). Zephyr takes precedence over SocketCAN. Where a hardware backend is not available its class still compiles but every operation returns `not_configured`.

### Endpoint differences
Only the endpoint passed to `open()` differs between platforms (`default_bus::endpoint_type`); everything else is source compatible.

| Backend | `endpoint_type` | Example | Notes |
|---|---|---|---|
| SocketCAN | `const char*` | `"can0"`, `"vcan0"`, `"any"` | Kernel interface name (shorter than `IFNAMSIZ`). `"any"` binds every CAN interface for receiving; sending then fails because the interface is unspecified. |
| Zephyr | `const struct device*` | `DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))` | Devicetree controller. Must be ready (`device_is_ready`), otherwise `not_configured`. |
| Mock | `const char*` | `"can0"` | Any non-empty name; it is only checked for null/empty. |

Other platform differences:

| Aspect | SocketCAN | Zephyr | Mock |
|---|---|---|---|
| Bit rate (`can_config::bitrate`) | Not applied; `ip link ... bitrate` owns it | Applied with `can_set_bitrate()` when non-zero; `0` keeps the devicetree `bus-speed` | Recorded only |
| `can_mode::loopback` | `CAN_RAW_RECV_OWN_MSGS` | Driver loopback mode | Sent frames are delivered to the node's own receive path |
| `can_mode::listen_only` | `invalid_config` (set `listen-only on` on the interface) | Driver listen-only mode | `send()` returns `invalid_config` |
| Filter slots | `MaxFilters` (default 8), up to `CAN_RAW_FILTER_MAX` | `MaxFilters` (default 8), limited by the controller (5 on the stock MCP2515 driver) | `MaxFilters` (default 8) |
| Receive buffering | Kernel socket buffer | Fixed queue of `RxQueueDepth` frames (default 16); overflow counted in `rx_overruns` | Fixed queue of `RxQueueDepth` (default 16) |
| Bus state / counters | From kernel error frames, current as of the last `receive()` | Read from the driver on every `query_status()` | Scripted with `set_bus_state()` |
| Threading | One sender and one receiver thread | One consumer thread for `receive()`; receive callback is the producer | Not thread safe |

## Header
`#include "castle_ext/io/can/can.hpp"`

`castle_ext/io/io.hpp` includes it as well. The umbrella includes `frame.hpp`, `filter.hpp`, `types.hpp`, `bus.hpp` and `backends/mock.hpp`, plus `backends/zephyr.hpp` or `backends/socket_can.hpp` when the matching macro is `1`. To use a specific backend that is not the default, include its header explicitly.

## Dependencies
- [`../../../core/config.md`](../../../core/config.md)
- [`../../../error/status.md`](../../../error/status.md)
- [`../../io/fd/control.md`](../../io/fd/control.md)
- [`../../io/fd/poll.md`](../../io/fd/poll.md)
- [`frame.md`](frame.md)
- [`filter.md`](filter.md)
- [`types.md`](types.md)
- [`bus.md`](bus.md)
- [`detail/slot_table.md`](detail/slot_table.md)
- [`backends/mock.md`](backends/mock.md)
- [`backends/socket_can.md`](backends/socket_can.md)
- [`backends/zephyr.md`](backends/zephyr.md)

## Public API
The umbrella adds three names to `castle::io::can`; everything else is documented on the linked pages.

| API | Description |
|---|---|
| `default_backend` | Type alias for the backend selected by the build: `zephyr::backend`, else `socket_can::backend`, else `mock::backend`. |
| `default_backend_is_hardware` | `bool` constant, true when `default_backend` drives real CAN hardware instead of the host-side mock. |
| `default_bus` | `basic_bus<default_backend>`: the bus facade for the current platform. |
| `frame`, `id_format`, `frame_type`, `validate_frame()` | See [`frame.md`](frame.md). |
| `filter`, `filter_handle`, `invalid_filter`, `validate_filter()` | See [`filter.md`](filter.md). |
| `timeout_type`, `no_wait`, `wait_forever`, `bitrate_*`, `can_mode`, `can_config`, `bus_state`, `bus_status` | See [`types.md`](types.md). |
| `basic_bus<Backend>`, `bus<Backend>` | See [`bus.md`](bus.md). |
| `mock::backend`, `socket_can::backend`, `zephyr::backend` | See [`backends/mock.md`](backends/mock.md), [`backends/socket_can.md`](backends/socket_can.md), [`backends/zephyr.md`](backends/zephyr.md). |

## Usage Example
See `samples/castle_ext/io/can/can.cpp`.

```cpp
#include "castle_ext/io/can/can.hpp"

#include <assert.h>

using namespace castle::io::can;

// Platform-independent application code: only the endpoint differs per target.
template <typename Bus>
castle::status echo_once(Bus& bus, typename Bus::endpoint_type endpoint)
{
    castle::status result = bus.open(endpoint, can_config(bitrate_500k));
    if (!castle::succeeded(result))
    {
        return result;
    }
    result = bus.accept_all();
    if (castle::succeeded(result))
    {
        frame request(0x123U);
        (void)request.set_data({0x01U, 0x02U});
        result = bus.send(request, castle::chrono::milliseconds(10));
    }
    (void)bus.close();
    return result;
}

int main()
{
    static_assert(default_backend_is_hardware || sizeof(default_bus) == sizeof(mock::backend),
                  "without hardware support default_bus is the mock");

    // Host-side run on the mock; on target use default_bus with "can0" or the Zephyr device.
    bus<mock::backend> node;
    assert(echo_once(node, "can0") == castle::status::ok);
    assert(!node.is_open());
    return 0;
}
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI, virtual functions or STL anywhere in the component; all queues and filter tables have compile-time capacity.
- Thread safety belongs to the backend (see "Endpoint differences"); the facade adds no locks.
- Hardware backends need their platform: SocketCAN needs a Linux POSIX build with the CAN UAPI headers, Zephyr needs Zephyr 3.2+ with `CONFIG_CAN=y`. Otherwise their operations return `not_configured`.
- Bit timing on Linux is an interface property set with `ip link`; on Zephyr it comes from `can_config::bitrate` or the devicetree `bus-speed`.
- Related: [`frame.md`](frame.md), [`filter.md`](filter.md), [`types.md`](types.md), [`bus.md`](bus.md), [`../../io/io.md`](../../io/io.md).

### Bringing up an MCP2515 on Linux (`can0`)
On a Raspberry Pi style board with the MCP2515 wired to SPI0 (16 MHz crystal, interrupt on GPIO 25), enable SPI and the kernel driver in `/boot/config.txt` (or `/boot/firmware/config.txt`), then reboot:

```
dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=25
```

The `mcp251x` driver then registers a `can0` network interface (check with `dmesg | grep -i mcp251x` and `ip link show can0`). Bit timing belongs to the interface, so set the bit rate when bringing it up:

```
sudo ip link set can0 up type can bitrate 500000
```

Useful variations: `ip link set can0 up type can bitrate 500000 listen-only on` for sniffing, `loopback on` for a self test, `restart-ms 100` for automatic bus-off recovery, `ip link set can0 down` before changing the bit rate. After that the application opens the interface by name:

```cpp
castle::io::can::default_bus bus;
bus.open("can0");              // can_config() default: bitrate is owned by the interface
bus.accept_all();
```

### Bringing up an MCP2515 on Zephyr
Zephyr 3.2 or newer is required (`can_start()`/`can_stop()` API). Enable the driver in `prj.conf`:

```
CONFIG_SPI=y
CONFIG_GPIO=y
CONFIG_CAN=y
CONFIG_CAN_MCP2515=y
```

Describe the controller in the board overlay: a `microchip,mcp2515` node under the SPI bus and select it as the `zephyr,canbus` chosen node. The pins, SPI controller label and clock below are an example; adapt them to your board.

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

The application takes the device from the devicetree:

```cpp
static const struct device* const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

castle::io::can::default_bus bus;
bus.open(can_dev, castle::io::can::can_config(castle::io::can::bitrate_500k));
bus.accept_all();
```

See [`backends/zephyr.md`](backends/zephyr.md) for the backend's queue and threading details.

### Portable semantics
These hold on every backend, so code and tests written against the mock behave the same on hardware:
- After `open()` **no filter is installed** and `receive()` yields nothing until `add_filter()` or `accept_all()` is called. `accept_all()` takes two filter slots (any standard plus any extended).
- A frame passes a filter when `frame.format == filter.format && ((frame.id ^ filter.id) & filter.mask) == 0`. Data and remote frames are not distinguished.
- `send()` returns once the frame is **queued**, not acknowledged. `full` means the queue stayed full for the whole timeout.
- `receive()` returns `empty` when no frame arrived in time. The output frame is only written on `ok`.
- Timeouts are non-negative (negative gives `invalid_argument`); `no_wait` never blocks; `wait_forever` blocks indefinitely.
- Calls on a closed bus return `not_configured`; opening twice returns `invalid_config`.
- Every call takes an optional `int* system_error`, cleared on entry.
- The facade validates configuration (`invalid_config`) and frame, filter and timeout arguments (`invalid_argument`) before the backend runs.

### Error handling
- Every operation returns `castle::status` (see [`error/status.md`](../../../error/status.md)); there are no exceptions. Test results with `castle::succeeded(...)`.
- Statuses the CAN API uses: `ok`, `full` (queue or filter table full), `empty` (nothing received in time), `out_of_range` (payload or DLC above 8), `not_found` (unknown filter handle), `not_configured` (closed, or backend unavailable on this build), `invalid_config` (bad configuration, already open, unsupported mode or filter), `invalid_argument` (bad frame, filter, timeout, endpoint), `system_call_error` (platform call failed), `data_loss` (truncated or unrepresentable frame). A poll timeout is never returned as `time_out`; it is mapped to `empty` or `full`.
- `system_call_error` carries the platform error in the optional `system_error` argument: `errno` on Linux (for example `ENODEV` for an unknown interface, `ENETDOWN` for an interface that went down) and the positive Zephyr driver errno (for example `ENETUNREACH` while bus-off). SocketCAN `send()` also reports `EAGAIN`/`ENOBUFS` there when it returns `full`; an `empty` `receive()` leaves it `0`. It is `0` for the other statuses. Zephyr driver codes `-EAGAIN`/`-ENOSPC` map to `full`, `-EINVAL` to `invalid_argument`, `-ENOTSUP`/`-ERANGE` to `invalid_config`, `-ENETDOWN` to `not_configured`.
- Fault confinement is observable through `query_status()` (`bus_state`, error counters, `rx_overruns`, `tx_failures`); see [`types.md`](types.md).

### CAN FD
CAN FD is out of scope. `frame` carries at most 8 payload bytes, MCP2515-class controllers are classic only, the backends never produce an FD frame. SocketCAN `receive()` reports a kernel frame it cannot represent (DLC above 8) or a short read as `data_loss`; the Zephyr receive callback silently drops frames with the FD flag or a DLC above 8.
