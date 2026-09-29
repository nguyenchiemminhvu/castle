# UBX protocol

## Overview
`ubx.hpp` provides the deterministic, header-only building blocks for the u-blox UBX binary protocol: framing constants, well-known message class/ID identifiers, little-endian field accessors, Fletcher-8 checksum calculation, a non-owning decoded-message view, and caller-buffer frame encode/decode. It contains no parsing state machine; that lives in [`ubx_parser`](../../parsers/ubx_parser/ubx_parser.md).

## Header
`#include "castle_ext/protocols/ubx/ubx.hpp"`

## Dependencies
- [`core/compiler.hpp`](../../../core/compiler.md)
- [`core/types.hpp`](../../../core/types.md)
- [`error/status.hpp`](../../../error/status.md)
- [`container/array_view.hpp`](../../../container/array_view.md)

## Public API
| API | Description |
| --- | --- |
| `UBX_SYNC_CHAR_1`, `UBX_SYNC_CHAR_2` | Frame synchronization bytes (`0xB5`, `0x62`). |
| `UBX_FRAME_OVERHEAD` | Fixed byte count outside the payload (8). |
| `UBX_MAX_PAYLOAD_LEN`, `UBX_SAFE_MAX_PAYLOAD_LEN` | 16-bit length-field ceiling and a practical embedded default (4096). |
| `UBX_CLASS_*`, `UBX_ID_*` | Well-known message class and message ID constants (NAV, RXM, INF, ACK, CFG, UPD, MON, TIM, ESF, SEC, NAV2, ...). |
| `message_header` | `msg_class`, `msg_id`, `payload_length`. |
| `checksum` | `ck_a`, `ck_b` pair. |
| `message_view` | Non-owning decoded message: `header`, `payload` (`array_view<const uint8_t>`), `check`. Accessors: `msg_class()`, `msg_id()`, `payload_length()`, `is(class, id)`, `key()`. |
| `checksum_accumulator` | Streaming Fletcher-8 accumulator: `reset()`, `update(byte)`, `value()`. |
| `read_le16/32/64(s)(data)` | Little-endian field readers (unsigned and signed). |
| `write_le16/32/64(data, value)` | Little-endian field writers. |
| `compute_checksum(data)` | Fletcher-8 checksum over a raw byte range. |
| `compute_frame_checksum(class, id, length, payload)` | Fletcher-8 checksum over class/ID/length/payload. |
| `frame_size(payload_length)` | Total encoded frame size for a given payload length. |
| `encode_frame(class, id, payload, output, capacity, written)` | Encodes one complete frame into a caller buffer. Returns `castle::status`. |
| `decode_frame(frame, output)` | Validates framing/checksum and builds a zero-copy `message_view` over `frame`. Returns `bool`. |
| `value_byte_size(key_id)` | Byte width encoded by a UBX CFG key ID's size code (0 when unknown). |

## Usage Example
See `samples/castle_ext/protocols/ubx/ubx.cpp`.

```cpp
#include "castle_ext/protocols/ubx/ubx.hpp"

uint8_t payload_data[2] = {0x01U, 0x02U};
castle::container::array_view<const uint8_t> payload(payload_data, 2U);

uint8_t frame[64U];
castle::size_type written = 0U;
castle_ext::protocols::ubx::encode_frame(
    castle_ext::protocols::ubx::UBX_CLASS_CFG,
    castle_ext::protocols::ubx::UBX_ID_CFG_VALSET,
    payload, frame, sizeof(frame), written);

castle_ext::protocols::ubx::message_view view;
castle_ext::protocols::ubx::decode_frame(
    castle::container::array_view<const uint8_t>(frame, written), view);
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers.
- `encode_frame()`/`decode_frame()` never read or write outside the caller-provided buffer.
- `message_view::payload` aliases the buffer passed to `decode_frame()` (or the parser's internal buffer when produced by `ubx_parser`); it is only valid while that storage is unchanged.
- `encode_frame()` rejects payloads larger than `UBX_MAX_PAYLOAD_LEN` (the UBX 16-bit length field ceiling), not `UBX_SAFE_MAX_PAYLOAD_LEN` (which is only the parser's default capacity).
- This header only defines protocol-level primitives for known message classes/IDs; it does not decode specific message payloads (e.g. NAV-PVT fields). Combine `message_view::payload` with `read_le16/32/64(s)` to decode specific messages.
