# UBX parser

## Overview
`ubx_parser<MaxPayload, CallbackStorageSize, CallbackStorageAlignment>` is a deterministic, byte-streaming parser for the UBX binary protocol. It runs the `WAIT_SYNC_1 -> WAIT_SYNC_2 -> CLASS -> ID -> LEN_LO -> LEN_HI -> PAYLOAD -> CK_A -> CK_B` state machine over caller-supplied bytes, discarding noise while resynchronizing, and reports each checksum-valid frame through a single message callback as a non-owning [`message_view`](../../protocols/ubx/ubx.md). A separate error callback reports bounded parsing errors (invalid sync, oversized payload, checksum mismatch, invalid input).

## Header
`#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"`

## Dependencies
- [`core/compiler.hpp`](../../../core/compiler.md)
- [`core/config.hpp`](../../../core/config.md)
- [`core/traits.hpp`](../../../core/traits.md)
- [`core/types.hpp`](../../../core/types.md)
- [`error/status.hpp`](../../../error/status.md)
- [`callbacks/function.hpp`](../../../callbacks/function.md)
- [`container/array.hpp`](../../../container/array.md)
- [`container/array_view.hpp`](../../../container/array_view.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../../protocols/ubx/ubx.md)

## Public API
| API | Description |
| --- | --- |
| `template <size_type MaxPayload = UBX_SAFE_MAX_PAYLOAD_LEN, size_type CallbackStorageSize = inplace_storage_reserved, size_type CallbackStorageAlignment = inplace_alignment_default> class ubx_parser` | Non-copyable, non-movable streaming parser with embedded payload storage. |
| `parse_state` | `wait_sync_1`, `wait_sync_2`, `wait_class`, `wait_id`, `wait_length_low`, `wait_length_high`, `read_payload`, `wait_checksum_a`, `wait_checksum_b`. |
| `parser_error_code` | `none`, `invalid_sync`, `payload_too_large`, `checksum_mismatch`, `invalid_argument`. |
| `parse_error` | `code`, `msg_class`, `msg_id`, `payload_length`. |
| `error_message(code)` | Stable human-readable description for a `parser_error_code`. |
| `set_message_callback(callback)` / `clear_message_callback()` / `has_message_callback()` | Install, clear, and query the per-frame message callback. |
| `set_error_callback(callback)` / `clear_error_callback()` / `has_error_callback()` | Install, clear, and query the error callback. |
| `feed(byte)` | Feeds exactly one byte. |
| `feed(data, size)` | Feeds a contiguous byte range. |
| `feed(array_view<const uint8_t>)` | Feeds a Castle array view. |
| `reset()` | Resets state machine and counters; keeps installed callbacks. |
| `static payload_capacity()` | Returns `MaxPayload`. |
| `state()`, `payload_size()` | Current state-machine state and partial-payload length. |
| `frames_decoded()`, `frames_discarded()` | Running counters. |

## Usage Example
See `samples/castle_ext/parsers/ubx_parser/ubx_parser.cpp`.

```cpp
#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"

castle_ext::parsers::ubx_parser::ubx_parser<> parser;

parser.set_message_callback(
    [](const castle_ext::protocols::ubx::message_view& message)
    {
        if (message.is(castle_ext::protocols::ubx::UBX_CLASS_NAV,
                        castle_ext::protocols::ubx::UBX_ID_NAV_PVT))
        {
            // Decode NAV-PVT fields directly from message.payload.
        }
    });

parser.set_error_callback(
    [](const castle_ext::parsers::ubx_parser::parse_error& error)
    {
        // Log castle_ext::parsers::ubx_parser::error_message(error.code).
    });

parser.feed(bytes, count);
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers. Callbacks use Castle's vtable-less `callbacks::function`.
- `MaxPayload` must be in `(0, UBX_MAX_PAYLOAD_LEN]`; `CallbackStorageSize` must be non-zero; `CallbackStorageAlignment` must be a non-zero power of two (enforced by `static_assert`).
- The parser is non-copyable and non-movable so embedded storage and callback state cannot be accidentally duplicated or relocated.
- Callbacks are invoked synchronously, before the triggering `feed()` call returns.
- `message_view::payload` passed to the message callback aliases the parser's internal payload buffer; it is valid only until the next byte is fed.
- A frame whose declared payload length exceeds `MaxPayload` is discarded as `payload_too_large` without ever touching the fixed-size buffer.
- All parser operations are single-threaded and non-reentrant; `feed()` must not be called recursively from within a callback.
