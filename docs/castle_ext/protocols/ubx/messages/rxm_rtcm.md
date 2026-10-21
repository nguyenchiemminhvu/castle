# UBX RXM-RTCM

## Overview

`rxm_rtcm` models UBX-RXM-RTCM (0x02 0x32), the 8-byte RTCM input status output. The receiver emits it once per successfully parsed RTCM input message, whether or not it supports that RTCM message. It reports the RTCM message type, the reference station ID, the u-blox 4072 subtype, whether the RTCM CRC failed and whether the receiver used the message. It is a [`fixed_message`](detail/fixed_message.md).

## Header

`#include "castle_ext/protocols/ubx/messages/rxm_rtcm.hpp"`

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
| `rxm_rtcm_msg_used : uint8_t` | `enum class` for `msgUsed` (`flags` bits 2..1): `unknown = 0` (do not know), `not_used = 1` (not used), `used = 2` (used successfully by the receiver). |
| `rxm_rtcm` | The message; `fixed_message<rxm_rtcm, UBX_CLASS_RXM, UBX_ID_RXM_RTCM, 8U>`. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `rxm_rtcm::msg_class` / `msg_id` | `UBX_CLASS_RXM` (`0x02`) / `UBX_ID_RXM_RTCM` (`0x32`) | Class/ID, inherited from `fixed_message`. |
| `rxm_rtcm::payload_length` | `8U` | Exact payload length. |
| `rxm_rtcm::FLAGS_CRC_FAILED` | `0x01U` (bit 0) | `crcFailed`. |
| `rxm_rtcm::FLAGS_MSG_USED` | `0x06U` (bits 2..1) | `msgUsed`. |
| `rxm_rtcm::REF_STATION_NOT_REPORTED` | `0xFFFFU` | `refStation` value reported for RTCM messages without a DF003 field. |

### Fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | - | Message version (0x02 for this version). Offset 0. |
| `flags` | `uint8_t` | - | Status flags, see `FLAGS_*`. Offset 1. |
| `sub_type` | `uint16_t` | - | Subtype of u-blox proprietary RTCM message 4072. Offset 2. |
| `ref_station` | `uint16_t` | - | Reference station ID, or `REF_STATION_NOT_REPORTED`. Offset 4. |
| `msg_type` | `uint16_t` | - | RTCM message type. Offset 6. |

### Functions

| Function | Description |
| --- | --- |
| `static bool rxm_rtcm::matches(const message_view& raw)` | Class/ID check (inherited). |
| `static bool rxm_rtcm::decode(const message_view& raw, rxm_rtcm& out)` | Inherited: requires class/ID and exactly 8 payload bytes; on failure `out` is value-initialised and `false` is returned. |
| `void rxm_rtcm::encode(payload_writer& writer) const` | Inherited; used by `encode_message()` (loop-back). |
| `bool rxm_rtcm::crc_failed() const` | `FLAGS_CRC_FAILED` set; `ref_station` / `msg_type` may then be corrupt. |
| `rxm_rtcm_msg_used rxm_rtcm::msg_used() const` | `(flags & FLAGS_MSG_USED) >> 1` as the enum. |

The complete RTCM input flow (port configuration, the RTCM frames, and this status message) is described in [RTCM 3.3 correction input to a u-blox receiver](../rtcm/rtcm_input.md).

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/rxm_rtcm.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    rxm_rtcm status{};
    if (!rxm_rtcm::decode(raw, status) || status.crc_failed())
    {
        return;
    }
    if (status.msg_used() == rxm_rtcm_msg_used::used)
    {
        // status.msg_type, e.g. 1077; status.ref_station unless REF_STATION_NOT_REPORTED
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/rxm_rtcm.cpp` for encode, decode and every accessor.

## Constraints & Notes

- Length validation: `decode()` requires exactly 8 payload bytes; shorter or longer payloads (or a different class/ID) return `false` with `out` value-initialised.
- `version` is stored, not checked: the decoder accepts any version byte.
- `msg_used()` casts the 2-bit field directly; the reserved wire value 3 yields an `rxm_rtcm_msg_used` value outside the named enumerators.
- When `crc_failed()` is `true`, `ref_station` and `msg_type` might be corrupted and misleading (per the spec).
- No heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
