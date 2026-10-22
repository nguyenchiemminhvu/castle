# CAN Types

## Overview
Platform-neutral CAN configuration, bus-state and timeout types shared by `basic_bus` and every backend, so application code keeps one vocabulary whether it runs on Linux SocketCAN, Zephyr or the host-side mock: `timeout_type` with the `no_wait`/`wait_forever` sentinels, common bit-rate constants, `can_mode`, `can_config` with `validate_config()`, `bus_state`, `bus_status` and `is_operational()`.

## Header
`#include "castle_ext/comm/can/types.hpp"`

## Dependencies
- [`../../../chrono/duration.md`](../../../chrono/duration.md)
- [`../../../core/compiler.md`](../../../core/compiler.md)
- [`../../../error/status.md`](../../../error/status.md)

## Public API
All names live in `castle::comm::can`.

### Timeouts
| API | Description |
|---|---|
| `timeout_type` | `castle::chrono::milliseconds`. Timeout of `send()` and `receive()`. |
| `no_wait` | `timeout_type(0)`. Perform the operation once and report `empty`/`full` if it cannot proceed. |
| `wait_forever` | `timeout_type::max()`. Block until the operation can proceed. |
| `is_forever(timeout_type)` | `constexpr bool`. True when `timeout.count() == wait_forever.count()`. |

A negative timeout is rejected by `basic_bus` with `invalid_argument`.

### Bit rates
| API | Description |
|---|---|
| `bitrate_10k`, `bitrate_20k`, `bitrate_50k`, `bitrate_100k`, `bitrate_125k`, `bitrate_250k`, `bitrate_500k`, `bitrate_800k`, `bitrate_1m` | `uint32_t` constants in bit/s (`10000` .. `1000000`). |
| `max_bitrate` | `uint32_t`, equal to `bitrate_1m`: highest classic-CAN bit rate accepted by `validate_config()`. |

### Configuration
| API | Description |
|---|---|
| `can_mode` | `uint8_t` enum: `normal` (transmit and receive), `loopback` (transmitted frames are looped back to this node and not driven onto the bus), `listen_only` (receive only, never drives the bus). |
| `can_config` | Fields `uint32_t bitrate`, `can_mode mode`. |
| `explicit can_config(uint32_t bit_rate = 0U, can_mode operating_mode = can_mode::normal)` | `constexpr`. Bit rate `0` keeps the driver/devicetree default. |
| `validate_config(can_config const&)` | Pure check, never touches hardware. Returns `ok`, or `invalid_config` when `bitrate > max_bitrate` or `mode` is not one of the three enumerators. `basic_bus::open()` calls it first. |

### Bus state
| API | Description |
|---|---|
| `bus_state` | `uint8_t` enum, ISO 11898-1 fault confinement: `stopped` (not open or controller stopped), `error_active` (normal), `error_warning` (an error counter reached 96), `error_passive` (an error counter reached 128), `bus_off` (transmit error counter exceeded 255; disconnected from the bus). |
| `bus_status` | Snapshot returned by `query_status()`: `bus_state state`, `uint8_t tx_error_counter`, `uint8_t rx_error_counter`, `uint32_t rx_overruns`, `uint32_t tx_failures`. |
| `bus_status()` | `constexpr`. `state = stopped`, all counters zero. |
| `is_operational(bus_state)` | `constexpr bool`. True for `error_active` and `error_warning` only; false for `stopped`, `error_passive` and `bus_off`. |

## Usage Example
See `samples/castle_ext/comm/can/types.cpp`.

```cpp
#include "castle_ext/comm/can/types.hpp"

#include <assert.h>

using namespace castle::comm::can;

int main()
{
    const can_config bus_speed(bitrate_500k);                       // normal mode
    const can_config self_test(bitrate_125k, can_mode::loopback);
    assert(validate_config(bus_speed) == castle::status::ok);
    assert(validate_config(self_test) == castle::status::ok);
    assert(validate_config(can_config(max_bitrate + 1U)) == castle::status::invalid_config);

    assert(!is_forever(no_wait));
    assert(is_forever(wait_forever));
    assert(timeout_type(250).count() == 250);

    const bus_status fresh;
    assert(fresh.state == bus_state::stopped);
    assert(fresh.rx_overruns == 0U && fresh.tx_failures == 0U);
    assert(is_operational(bus_state::error_warning));
    assert(!is_operational(bus_state::error_passive));
    assert(!is_operational(bus_state::bus_off));
    return 0;
}
```

## Constraints & Notes
- Everything here is a `constexpr`-friendly value type or constant: no heap, exceptions, RTTI or virtual functions.
- `bitrate` is applied only by backends that own bit timing: Zephyr calls `can_set_bitrate()` when it is non-zero. On Linux the interface owns bit timing (`ip link set can0 type can bitrate 500000`), so the SocketCAN backend validates but does not apply it; the mock backend just records it.
- Supported modes differ per platform: the SocketCAN backend implements `loopback` with `CAN_RAW_RECV_OWN_MSGS` and rejects `listen_only` with `invalid_config` (set `listen-only on` on the interface instead). The mock backend refuses `send()` in `listen_only` with `invalid_config`. Zephyr passes the mode to the driver, which may reject it.
- `bus_status` counters: `rx_overruns` and `tx_failures` count dropped receive frames and failed transmissions. The error counters are filled where the platform reports them (always on Zephyr; on SocketCAN only when the kernel sends them in an error frame, and then only as of the last `receive()`).
- `is_operational()` is a state predicate, not a health check: `error_passive` can still transmit but signals errors passively, and is treated as not operational here.
- `timeout_type` is millisecond-granular. Backends clamp very large values to their platform's limit (`INT_MAX` ms for `poll()`, `INT32_MAX` ms on Zephyr); use `wait_forever` for "no limit".
- Related: [`frame.md`](frame.md), [`filter.md`](filter.md), [`bus.md`](bus.md), [`can.md`](can.md).
