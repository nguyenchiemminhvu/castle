# UBX TIM-TP

## Overview

`tim_tp` models UBX-TIM-TP (0x0D 0x01), the periodic/polled output describing the timing of the next pulse on the TIMEPULSE0 output: its time of week (integer and 2^-32 ms sub-millisecond part), week number, quantization error, time base, (T)RAIM state and time reference. The spec recommends setting both the measurement rate (CFG-RATE) and the time pulse frequency (CFG-TP) to 1 Hz when using this message. It is a [`fixed_message`](detail/fixed_message.md).

## Header

`#include "castle_ext/protocols/ubx/messages/tim_tp.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/constants.hpp`](../../../../core/constants.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- [`castle_ext/protocols/ubx/messages/nav_timeutc.hpp`](nav_timeutc.md)
- `stdint.h`

## Public API

All names live in `castle::protocols::ubx::messages`.

### Types

| Type | Description |
| --- | --- |
| `tim_tp_time_base : uint8_t` | `enum class` for `timeBase` (`flags` bit 0): `gnss = 0`, `utc = 1`. |
| `tim_tp_raim : uint8_t` | `enum class` for `raim` (`flags` bits 3..2): `not_available = 0` (information not available), `not_active = 1`, `active = 2`. |
| `tim_tp_time_ref_gnss : uint8_t` | `enum class` for `timeRefGnss` (`refInfo` bits 3..0, only valid when the time base is GNSS): `gps = 0`, `glonass = 1`, `beidou = 2`, `galileo = 3`, `unknown = 15`. |
| `nav_timeutc_utc_standard` | Reused from [`nav_timeutc.hpp`](nav_timeutc.md) for `utcStandard` (`refInfo` bits 7..4, only valid when the time base is UTC): `not_available = 0`, `crl = 1`, `nist = 2`, `usno = 3`, `bipm = 4`, `european_laboratories = 5`, `former_soviet_union = 6`, `ntsc = 7`, `unknown = 15`. |
| `tim_tp` | The message; `fixed_message<tim_tp, UBX_CLASS_TIM, UBX_ID_TIM_TP, 16U>`. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `tim_tp::msg_class` / `msg_id` | `UBX_CLASS_TIM` (`0x0D`) / `UBX_ID_TIM_TP` (`0x01`) | Class/ID, inherited from `fixed_message`. |
| `tim_tp::payload_length` | `16U` | Exact payload length. |
| `tim_tp::FLAGS_TIME_BASE` | `0x01U` (bit 0) | `timeBase`: 0 = GNSS, 1 = UTC. |
| `tim_tp::FLAGS_UTC` | `0x02U` (bit 1) | `utc`: UTC available. |
| `tim_tp::FLAGS_RAIM` | `0x0CU` (bits 3..2) | `raim`. |
| `tim_tp::FLAGS_Q_ERR_INVALID` | `0x10U` (bit 4) | `qErrInvalid`: quantization error invalid. |
| `tim_tp::REF_INFO_TIME_REF_GNSS` | `0x0FU` (bits 3..0) | `timeRefGnss`. |
| `tim_tp::REF_INFO_UTC_STANDARD` | `0xF0U` (bits 7..4) | `utcStandard`. |

### Fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `tow_ms` | `uint32_t` | ms | Time pulse time of week according to the time base. Offset 0. |
| `tow_sub_ms` | `uint32_t` | 2^-32 ms | Submillisecond part of `tow_ms`. Offset 4. |
| `q_err` | `int32_t` | ps | Quantization error of the time pulse. Offset 8. |
| `week` | `uint16_t` | weeks | Time pulse week number according to the time base. Offset 12. |
| `flags` | `uint8_t` | - | Flags, see `FLAGS_*`. Offset 14. |
| `ref_info` | `uint8_t` | - | Time reference information, see `REF_INFO_*`. Offset 15. |

### Functions

| Function | Description |
| --- | --- |
| `static bool tim_tp::matches(const message_view& raw)` | Class/ID check (inherited). |
| `static bool tim_tp::decode(const message_view& raw, tim_tp& out)` | Inherited: requires class/ID and exactly 16 payload bytes; on failure `out` is value-initialised and `false` is returned. |
| `void tim_tp::encode(payload_writer& writer) const` | Inherited; used by `encode_message()` (loop-back). |
| `tim_tp_time_base tim_tp::time_base() const` | `flags & FLAGS_TIME_BASE` as the enum. |
| `bool tim_tp::utc_available() const` | `FLAGS_UTC` set. |
| `tim_tp_raim tim_tp::raim() const` | `(flags & FLAGS_RAIM) >> 2` as the enum. |
| `bool tim_tp::q_err_invalid() const` | `FLAGS_Q_ERR_INVALID` set. |
| `tim_tp_time_ref_gnss tim_tp::time_ref_gnss() const` | `ref_info & REF_INFO_TIME_REF_GNSS` as the enum. |
| `nav_timeutc_utc_standard tim_tp::utc_standard() const` | `(ref_info & REF_INFO_UTC_STANDARD) >> 4` as the enum. |
| `double tim_tp::tow_sub_ms_ms() const` | `tow_sub_ms × 2^-32` [ms], always in [0, 1). |
| `double tim_tp::q_err_ns() const` | `q_err × 10^-3` [ns]. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/tim_tp.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    tim_tp pulse{};
    if (!tim_tp::decode(raw, pulse))
    {
        return;
    }
    const double tow_ms = static_cast<double>(pulse.tow_ms) + pulse.tow_sub_ms_ms();
    if (!pulse.q_err_invalid())
    {
        const double correction_ns = pulse.q_err_ns();
        // apply correction_ns to the pulse at week pulse.week, tow_ms
    }
    if (pulse.time_base() == tim_tp_time_base::gnss)
    {
        // pulse.time_ref_gnss() is valid
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/tim_tp.cpp` for encode, decode, every accessor and both helpers.

## Constraints & Notes

- Length validation: `decode()` requires exactly 16 payload bytes; shorter or longer payloads (or a different class/ID) return `false` with `out` value-initialised.
- `utc_standard()` reuses `nav_timeutc_utc_standard` from `nav_timeutc.hpp` instead of a second, identical enumeration; including `tim_tp.hpp` therefore also pulls in the NAV-TIMEUTC model.
- Field names are unchanged from the previous version.
- `time_ref_gnss()` is only meaningful when `time_base()` is `gnss`, `utc_standard()` only when it is `utc` (per the spec). Undefined wire values (e.g. `raim` 3, `timeRefGnss` 4..14) are cast directly and yield values outside the named enumerators.
- Scaled helpers convert on demand; decode keeps the exact raw integers. The 2^-32 scale is exact in `double`.
- `encode_message()` returns `castle::status::full` when the output buffer is smaller than the 24-byte frame.
- No heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
