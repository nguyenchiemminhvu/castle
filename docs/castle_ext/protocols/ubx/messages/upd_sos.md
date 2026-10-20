# UBX UPD-SOS

## Overview

`upd_sos.hpp` models UBX-UPD-SOS (0x09 0x14), save-on-shutdown: the host can save part of the battery-backed memory (BBR) into a file in flash before switching off the supply, emulating a backup battery. One class/ID carries five messages:

- Poll backup restore status (poll request, empty payload): no struct; send it with `encode_poll<upd_sos_restored>(buf, cap, written)`. The receiver answers with `upd_sos_restored`.
- `upd_sos_save`: create backup in flash (command, `cmd` 0, 4 bytes).
- `upd_sos_clear`: clear backup in flash (command, `cmd` 1, 4 bytes).
- `upd_sos_ack`: backup creation acknowledge (output, `cmd` 2, 8 bytes).
- `upd_sos_restored`: system restored from backup (output, `cmd` 3, 8 bytes).

The four payload layouts are [`typed_message`](detail/typed_message.md)s, told apart by `cmd` (payload byte 0) plus the exact length, so a frame on 0x09 0x14 matches at most one of them.

## Header

`#include "castle_ext/protocols/ubx/messages/upd_sos.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/typed_message.hpp`](detail/typed_message.md)
- `stdint.h`

## Public API

All names live in `castle::protocols::ubx::messages`.

### Types

| Type | Description |
| --- | --- |
| `upd_sos_cmd : uint8_t` | `enum class` for the `cmd` byte: `save = 0x00` (create backup, command), `clear = 0x01` (clear backup, command), `ack = 0x02` (backup creation acknowledge, output), `restored = 0x03` (system restored from backup, output). |
| `upd_sos_ack_response : uint8_t` | `enum class` for `response` of `upd_sos_ack`: `not_acknowledged = 0x00`, `acknowledged = 0x01`. |
| `upd_sos_restore_response : uint8_t` | `enum class` for `response` of `upd_sos_restored`: `unknown = 0x00`, `failed = 0x01` (failed restoring from backup), `restored = 0x02` (restored from backup), `not_restored = 0x03` (not restored, no backup). |
| `upd_sos_save` | `typed_message<upd_sos_save, UBX_CLASS_UPD, UBX_ID_UPD_SOS, 0x00, 4U>`; no data members. |
| `upd_sos_clear` | `typed_message<upd_sos_clear, UBX_CLASS_UPD, UBX_ID_UPD_SOS, 0x01, 4U>`; no data members. |
| `upd_sos_ack` | `typed_message<upd_sos_ack, UBX_CLASS_UPD, UBX_ID_UPD_SOS, 0x02, 8U>`. |
| `upd_sos_restored` | `typed_message<upd_sos_restored, UBX_CLASS_UPD, UBX_ID_UPD_SOS, 0x03, 8U>`; also the answer to the poll. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `msg_class` / `msg_id` (all four) | `UBX_CLASS_UPD` (`0x09`) / `UBX_ID_UPD_SOS` (`0x14`) | Class/ID, inherited from `fixed_message`. |
| `upd_sos_save::message_type` / `upd_sos_clear::message_type` / `upd_sos_ack::message_type` / `upd_sos_restored::message_type` | `0x00U` / `0x01U` / `0x02U` / `0x03U` | `cmd` byte, inherited from `typed_message`. |
| `upd_sos_save::payload_length` / `upd_sos_clear::payload_length` | `4U` | Exact payload length of the commands. |
| `upd_sos_ack::payload_length` / `upd_sos_restored::payload_length` | `8U` | Exact payload length of the outputs. |

### `upd_sos_save` / `upd_sos_clear` fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| (cmd) | `uint8_t` | - | Not a member: written as the layout's `upd_sos_cmd` value and re-validated on decode. Payload offset 0, followed by 3 reserved bytes. |

### `upd_sos_ack` fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| (cmd) | `uint8_t` | - | Not a member: `upd_sos_cmd::ack`, re-validated on decode. Payload offset 0, followed by 3 reserved bytes. |
| `response` | `upd_sos_ack_response` | - | Backup creation result; default `not_acknowledged`. Payload offset 4, followed by 3 reserved bytes. |

### `upd_sos_restored` fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| (cmd) | `uint8_t` | - | Not a member: `upd_sos_cmd::restored`, re-validated on decode. Payload offset 0, followed by 3 reserved bytes. |
| `response` | `upd_sos_restore_response` | - | Restore result; default `unknown`. Payload offset 4, followed by 3 reserved bytes. |

### Functions

| Function | Description |
| --- | --- |
| `static bool matches(const message_view& raw)` (all four) | Inherited from `typed_message`: `true` for class/ID 0x09 0x14 with the layout's exact length and `cmd` byte. An empty (poll) frame matches none of them. |
| `static bool decode(const message_view& raw, upd_sos_X& out)` (all four) | Inherited from `fixed_message`: requires class/ID, the exact length and the `cmd` byte. On failure `out` is value-initialised and `false` is returned. |
| `void encode(payload_writer& writer) const` (all four) | Inherited; writes the `cmd` byte and zero reserved bytes. Used by `encode_message()`. |
| `encode_poll<upd_sos_restored>(uint8_t* output, castle::size_type capacity, castle::size_type& written)` | Free function from the UBX writer: writes the empty poll frame on 0x09 0x14. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/upd_sos.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

castle::status request_backup(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    // Before power-off: save the BBR into flash (12-byte frame).
    return encode_message(upd_sos_save{}, frame, capacity, written);
}

castle::status poll_restore_status(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    // After power-on: poll the restore status (empty payload, 8-byte frame).
    return encode_poll<upd_sos_restored>(frame, capacity, written);
}

void handle(const message_view& raw)
{
    upd_sos_ack ack{};
    upd_sos_restored restored{};
    if (upd_sos_ack::decode(raw, ack) && (ack.response == upd_sos_ack_response::acknowledged))
    {
        // safe to switch off the supply
    }
    else if (upd_sos_restored::decode(raw, restored)
             && (restored.response == upd_sos_restore_response::restored))
    {
        // send upd_sos_clear{} to erase the backup file
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/upd_sos.cpp` for every layout, the poll, the loop-back decodes and the cross-layout rejection.

## Constraints & Notes

- Length validation: each layout accepts only its exact length (4 for save/clear, 8 for ack/restored) together with its `cmd` byte; any other length, `cmd` byte or class/ID makes `decode()` return `false` with `out` value-initialised, so a frame decodes as at most one layout.
- Poll: the poll request has an empty payload and therefore no struct; send it with `encode_poll<upd_sos_restored>(buf, cap, written)`. The receiver answers with `upd_sos_restored` (8 bytes), which is also sent unsolicited after a restore.
- The `response` byte means different things in the two outputs, so `upd_sos_ack_response` and `upd_sos_restore_response` differ by design (e.g. value 1 is `acknowledged` in one and `failed` in the other). Wire values outside the named enumerators are stored as-is.
- The spec recommends a GNSS stop via UBX-CFG-RST before `upd_sos_save` to keep the BBR consistent, and clearing the backup with `upd_sos_clear` after the restore has been reported.
- `encode_message()` returns `castle::status::full` when the output buffer is too small (12-byte frame for save/clear, 16 bytes for ack/restored, 8 bytes for the poll).
- No heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.

## Migration

| Previous | Now |
| --- | --- |
| `upd_sos_output` | Removed; decode with `upd_sos_ack` or `upd_sos_restored`. |
| `UPD_SOS_CMD_SAVE` | `upd_sos_cmd::save` |
| `UPD_SOS_CMD_CLEAR` | `upd_sos_cmd::clear` |
| `UPD_SOS_CMD_ACK` | `upd_sos_cmd::ack` |
| `UPD_SOS_CMD_RESTORE` | `upd_sos_cmd::restored` |
| `UPD_SOS_RESP_NOT_ACK` | `upd_sos_ack_response::not_acknowledged` |
| `UPD_SOS_RESP_ACK` | `upd_sos_ack_response::acknowledged` |
