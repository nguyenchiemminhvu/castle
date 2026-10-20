# UBX RXM-PMREQ

## Overview

`rxm_pmreq.hpp` models UBX-RXM-PMREQ (0x02 0x41), the power management request the host sends to put the receiver into backup mode for a given duration. It is a **Command**, so building a value and passing it to `encode_message()` is the primary API; `decode()` is provided for loop-back and logging.

Two layouts share the class/ID and differ only by payload length (there is no type byte):

- `rxm_pmreq_v0`: 8 bytes, `duration` + `flags` (backup).
- `rxm_pmreq_v1`: 16 bytes, adds the version byte (0x00), the `force` flag and the wake-up sources.

Each is a [`fixed_message`](detail/fixed_message.md) whose `matches()` is narrowed to its exact length, so a frame on 0x02 0x41 matches exactly one of the two.

## Header

`#include "castle_ext/protocols/ubx/messages/rxm_pmreq.hpp"`

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
| `rxm_pmreq_v0` | 8-byte command payload (no version, no wake-up sources); `fixed_message<rxm_pmreq_v0, UBX_CLASS_RXM, UBX_ID_RXM_PMREQ, 8U>`. |
| `rxm_pmreq_v1` | 16-byte command payload (version 0x00, with wake-up sources); `fixed_message<rxm_pmreq_v1, UBX_CLASS_RXM, UBX_ID_RXM_PMREQ, 16U>`. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `RXM_PMREQ_DURATION_INFINITE` | `0U` | `duration` value requesting an infinite task duration (default of both models). |
| `RXM_PMREQ_VERSION` | `0x00U` | Version byte of the 16-byte layout. |
| `msg_class` / `msg_id` (both) | `UBX_CLASS_RXM` (`0x02`) / `UBX_ID_RXM_PMREQ` (`0x41`) | Class/ID, inherited from `fixed_message`. |
| `rxm_pmreq_v0::payload_length` / `rxm_pmreq_v1::payload_length` | `8U` / `16U` | Exact payload lengths. |
| `rxm_pmreq_v0::FLAGS_BACKUP` / `rxm_pmreq_v1::FLAGS_BACKUP` | `0x00000002U` (bit 1) | `backup`: enter backup mode for `duration`. |
| `rxm_pmreq_v1::FLAGS_FORCE` | `0x00000004U` (bit 2) | `force`: back up even with USB connected. |
| `rxm_pmreq_v1::WAKEUP_UARTRX` | `0x00000008U` (bit 3) | Wake up on an edge on UART RX. |
| `rxm_pmreq_v1::WAKEUP_EXTINT0` | `0x00000020U` (bit 5) | Wake up on an edge on EXTINT0. |
| `rxm_pmreq_v1::WAKEUP_EXTINT1` | `0x00000040U` (bit 6) | Wake up on an edge on EXTINT1. |
| `rxm_pmreq_v1::WAKEUP_SPICS` | `0x00000080U` (bit 7) | Wake up on an edge on SPI CS. |

### `rxm_pmreq_v0` fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `duration` | `uint32_t` | ms | Task duration, 0 = infinite, at most 12 days. Payload offset 0. |
| `flags` | `uint32_t` | - | Task flags, see `FLAGS_*`. Payload offset 4. |

### `rxm_pmreq_v1` fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| (version) | `uint8_t` | - | Not a member: written as `RXM_PMREQ_VERSION` and re-validated on decode. Payload offset 0, followed by 3 reserved bytes. |
| `duration` | `uint32_t` | ms | Task duration, 0 = infinite, at most 12 days. Payload offset 4. |
| `flags` | `uint32_t` | - | Task flags, see `FLAGS_*`. Payload offset 8. |
| `wakeup_sources` | `uint32_t` | - | Wake-up pins, see `WAKEUP_*`. Payload offset 12. |

### Functions

| Function | Description |
| --- | --- |
| `static bool rxm_pmreq_v0::matches(const message_view& raw)` | `true` for class/ID 0x02 0x41 with exactly 8 payload bytes. |
| `static bool rxm_pmreq_v1::matches(const message_view& raw)` | `true` for class/ID 0x02 0x41 with exactly 16 payload bytes. |
| `static bool decode(const message_view& raw, rxm_pmreq_vN& out)` | Inherited from `fixed_message`: requires class/ID and the exact length; for v1 also the version byte 0x00. On failure `out` is value-initialised and `false` is returned. |
| `void encode(payload_writer& writer) const` | Inherited from `fixed_message`; used by `encode_message()`. v1 writes the version byte 0x00 and zero reserved bytes. |
| `bool rxm_pmreq_v0::backup() const` / `rxm_pmreq_v1::backup() const` | `FLAGS_BACKUP` set. |
| `bool rxm_pmreq_v1::force() const` | `FLAGS_FORCE` set. |
| `bool rxm_pmreq_v1::wakeup_uartrx() const` | `WAKEUP_UARTRX` set. |
| `bool rxm_pmreq_v1::wakeup_extint0() const` | `WAKEUP_EXTINT0` set. |
| `bool rxm_pmreq_v1::wakeup_extint1() const` | `WAKEUP_EXTINT1` set. |
| `bool rxm_pmreq_v1::wakeup_spics() const` | `WAKEUP_SPICS` set. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/rxm_pmreq.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

rxm_pmreq_v1 request{};
request.duration = 60000U; // 60 s
request.flags = rxm_pmreq_v1::FLAGS_BACKUP;
request.wakeup_sources = rxm_pmreq_v1::WAKEUP_UARTRX | rxm_pmreq_v1::WAKEUP_EXTINT0;

uint8_t frame[24U]; // 6 header + 16 payload + 2 checksum
castle::size_type written = 0U;
const castle::status result = encode_message(request, frame, sizeof(frame), written);
```

See `samples/castle_ext/protocols/ubx/messages/rxm_pmreq.cpp` for both layouts, the loop-back decode and every accessor.

## Constraints & Notes

- Length validation: the layouts differ by length only. Each `matches()` checks the exact length (8 or 16), and `decode()` rejects any other length, so an 8-byte frame never decodes as v1 and vice versa.
- v1 version byte: fixed to `0x00` (`RXM_PMREQ_VERSION`); `encode()` always writes it and `decode()` re-validates it (a different byte makes `decode()` return `false`).
- Command message: encode is the primary API; `decode()` exists for loop-back. Per the spec, `backup` only takes effect when the receiver is not connected to USB unless `force` (v1 only) is also set, which disables the USB interface.
- `encode_message()` returns `castle::status::full` when the output buffer is too small (frame size 16 bytes for v0, 24 bytes for v1).
- No heap allocation, no exceptions.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
