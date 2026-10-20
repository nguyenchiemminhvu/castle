# UBX TIM-TM2

## Overview

`tim_tm2` models UBX-TIM-TM2 (0x0D 0x03), the periodic/polled time mark output used for high-precision time stamping and pulse counting on an EXTINT channel. It reports the rising and falling edge times (week, millisecond time of week and nanosecond fraction), a rising edge counter and an accuracy estimate. The delay figures and time base configured with the CFG-TP items also apply to this message. It is a [`fixed_message`](detail/fixed_message.md).

## Header

`#include "castle_ext/protocols/ubx/messages/tim_tm2.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

All names live in `castle::protocols::ubx::messages`.

### Types

| Type | Description |
| --- | --- |
| `tim_tm2_time_base : uint8_t` | `enum class` for `timeBase` (`flags` bits 4..3): `receiver = 0` (receiver time), `gnss = 1` (GNSS time, system configured in CFG-TP for tpIdx=0), `utc = 2` (UTC, variant configured in CFG-NAVSPG-*). |
| `tim_tm2` | The message; `fixed_message<tim_tm2, UBX_CLASS_TIM, UBX_ID_TIM_TM2, 28U>`. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `tim_tm2::msg_class` / `msg_id` | `UBX_CLASS_TIM` (`0x0D`) / `UBX_ID_TIM_TM2` (`0x03`) | Class/ID, inherited from `fixed_message`. |
| `tim_tm2::payload_length` | `28U` | Exact payload length. |
| `tim_tm2::FLAGS_MODE` | `0x01U` (bit 0) | `mode`: 0 = single, 1 = running. |
| `tim_tm2::FLAGS_RUN` | `0x02U` (bit 1) | `run`: 0 = armed, 1 = stopped. |
| `tim_tm2::FLAGS_NEW_FALLING_EDGE` | `0x04U` (bit 2) | `newFallingEdge`: new falling edge detected. |
| `tim_tm2::FLAGS_TIME_BASE` | `0x18U` (bits 4..3) | `timeBase`. |
| `tim_tm2::FLAGS_UTC` | `0x20U` (bit 5) | `utc`: UTC available. |
| `tim_tm2::FLAGS_TIME` | `0x40U` (bit 6) | `time`: time valid (valid GNSS fix). |
| `tim_tm2::FLAGS_NEW_RISING_EDGE` | `0x80U` (bit 7) | `newRisingEdge`: new rising edge detected. |

### Fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `ch` | `uint8_t` | - | Channel (EXTINT) on which the pulse was measured. Offset 0. |
| `flags` | `uint8_t` | - | Bitmask, see `FLAGS_*`. Offset 1. |
| `count` | `uint16_t` | - | Rising edge counter. Offset 2. |
| `wn_r` | `uint16_t` | weeks | Week number of the last rising edge. Offset 4. |
| `wn_f` | `uint16_t` | weeks | Week number of the last falling edge. Offset 6. |
| `tow_ms_r` | `uint32_t` | ms | Time of week of the rising edge. Offset 8. |
| `tow_sub_ms_r` | `uint32_t` | ns | Millisecond fraction of the rising-edge time of week. Offset 12. |
| `tow_ms_f` | `uint32_t` | ms | Time of week of the falling edge. Offset 16. |
| `tow_sub_ms_f` | `uint32_t` | ns | Millisecond fraction of the falling-edge time of week. Offset 20. |
| `acc_est` | `uint32_t` | ns | Accuracy estimate. Offset 24. |

### Functions

| Function | Description |
| --- | --- |
| `static bool tim_tm2::matches(const message_view& raw)` | Class/ID check (inherited). |
| `static bool tim_tm2::decode(const message_view& raw, tim_tm2& out)` | Inherited: requires class/ID and exactly 28 payload bytes; on failure `out` is value-initialised and `false` is returned. |
| `void tim_tm2::encode(payload_writer& writer) const` | Inherited; used by `encode_message()` (loop-back). |
| `bool tim_tm2::running() const` | `FLAGS_MODE` set: `true` = running, `false` = single. |
| `bool tim_tm2::stopped() const` | `FLAGS_RUN` set: `true` = stopped, `false` = armed. |
| `bool tim_tm2::new_falling_edge() const` | `FLAGS_NEW_FALLING_EDGE` set. |
| `tim_tm2_time_base tim_tm2::time_base() const` | `(flags & FLAGS_TIME_BASE) >> 3` as the enum. |
| `bool tim_tm2::utc_available() const` | `FLAGS_UTC` set. |
| `bool tim_tm2::time_valid() const` | `FLAGS_TIME` set. |
| `bool tim_tm2::new_rising_edge() const` | `FLAGS_NEW_RISING_EDGE` set. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/tim_tm2.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    tim_tm2 mark{};
    if (!tim_tm2::decode(raw, mark) || !mark.time_valid())
    {
        return;
    }
    if (mark.new_rising_edge() && (mark.time_base() == tim_tm2_time_base::gnss))
    {
        // edge at week mark.wn_r, mark.tow_ms_r ms + mark.tow_sub_ms_r ns
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/tim_tm2.cpp` for encode, decode and every accessor.

## Constraints & Notes

- Length validation: `decode()` requires exactly 28 payload bytes; shorter or longer payloads (or a different class/ID) return `false` with `out` value-initialised.
- `time_base()` casts the 2-bit field directly; the undefined wire value 3 yields a `tim_tm2_time_base` value outside the named enumerators.
- The sub-millisecond fields are integer nanoseconds; no scaled helpers are needed.
- `encode_message()` returns `castle::status::full` when the output buffer is smaller than the 36-byte frame.
- No heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
