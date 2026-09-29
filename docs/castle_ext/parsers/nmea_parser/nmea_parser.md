# NMEA parser

## Overview
`nmea_parser<MaxSentenceLen, MaxFields, CallbackStorageSize, CallbackStorageAlignment>` is a deterministic, byte-streaming parser for the NMEA 0183 text protocol. It runs the `WAIT_DOLLAR -> ACCUMULATE -> [WAIT_CHECKSUM_HI -> WAIT_CHECKSUM_LO] -> WAIT_LF` state machine over caller-supplied bytes, discarding noise while resynchronizing on `$`, and reports each framed sentence through a single message callback as a non-owning [`message_view`](../../protocols/nmea/nmea.md). A separate error callback reports bounded parsing errors (checksum mismatch, oversized sentence, unexpected `$`, invalid input).

## Header
`#include "castle_ext/parsers/nmea_parser/nmea_parser.hpp"`

## Dependencies
- [`core/compiler.hpp`](../../../core/compiler.md)
- [`core/config.hpp`](../../../core/config.md)
- [`core/traits.hpp`](../../../core/traits.md)
- [`core/types.hpp`](../../../core/types.md)
- [`error/status.hpp`](../../../error/status.md)
- [`callbacks/function.hpp`](../../../callbacks/function.md)
- [`container/array.hpp`](../../../container/array.md)
- [`container/array_view.hpp`](../../../container/array_view.md)
- [`container/string_view.hpp`](../../../container/string_view.md)
- [`castle_ext/protocols/nmea/nmea.hpp`](../../protocols/nmea/nmea.md)

## Public API
| API | Description |
| --- | --- |
| `template <size_type MaxSentenceLen = NMEA_SAFE_MAX_SENTENCE_LEN, size_type MaxFields = NMEA_DEFAULT_MAX_FIELDS, size_type CallbackStorageSize = inplace_storage_reserved, size_type CallbackStorageAlignment = inplace_alignment_default> class nmea_parser` | Non-copyable, non-movable streaming parser with embedded content and field-table storage. |
| `parse_state` | `wait_dollar`, `accumulate`, `wait_checksum_hi`, `wait_checksum_lo`, `wait_lf`. |
| `parser_error_code` | `none`, `checksum_mismatch`, `sentence_too_long`, `unexpected_start_in_sentence`, `invalid_argument`. |
| `parse_error` | `code`, `talker`, `type` (talker/type populated only for `checksum_mismatch`). |
| `error_message(code)` | Stable human-readable description for a `parser_error_code`. |
| `set_message_callback(callback)` / `clear_message_callback()` / `has_message_callback()` | Install, clear, and query the per-sentence message callback. |
| `set_error_callback(callback)` / `clear_error_callback()` / `has_error_callback()` | Install, clear, and query the error callback. |
| `feed(byte)` | Feeds exactly one character. |
| `feed(data, size)` | Feeds a contiguous `char` range. |
| `feed(string_view)` | Feeds a Castle string view. |
| `feed(data, size)` (`uint8_t`) / `feed(array_view<const uint8_t>)` | Convenience overloads for raw transport byte buffers. |
| `reset()` | Resets state machine and counters; keeps installed callbacks. |
| `static max_sentence_length()`, `static max_fields()` | Returns `MaxSentenceLen` / `MaxFields`. |
| `state()`, `buffered_length()` | Current state-machine state and partial-content length. |
| `messages_decoded()`, `messages_discarded()` | Running counters. |

## Usage Example
See `samples/castle_ext/parsers/nmea_parser/nmea_parser.cpp`.

```cpp
#include "castle_ext/parsers/nmea_parser/nmea_parser.hpp"

castle_ext::parsers::nmea_parser::nmea_parser<> parser;

parser.set_message_callback(
    [](const castle_ext::protocols::nmea::message_view& sentence)
    {
        if (sentence.is(castle_ext::protocols::nmea::SENTENCE_GGA))
        {
            // Decode fields directly via castle_ext::protocols::nmea::parse_*.
        }
    });

parser.set_error_callback(
    [](const castle_ext::parsers::nmea_parser::parse_error& error)
    {
        // Log castle_ext::parsers::nmea_parser::error_message(error.code).
    });

parser.feed(bytes, count);
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers. Callbacks use Castle's vtable-less `callbacks::function`.
- `MaxSentenceLen` and `MaxFields` must be non-zero; `CallbackStorageSize` must be non-zero; `CallbackStorageAlignment` must be a non-zero power of two (enforced by `static_assert`).
- The parser is non-copyable and non-movable so embedded storage and callback state cannot be accidentally duplicated or relocated.
- Callbacks are invoked synchronously, before the triggering `feed()` call returns.
- `message_view::talker`/`type`/`fields` passed to the message callback alias the parser's internal buffers; valid only until the next byte is fed.
- An unexpected `$` seen mid-sentence is reported as `unexpected_start_in_sentence` and parsing immediately resumes on that `$` (the byte is not lost). An oversized sentence or an invalid checksum hex pair discards back to `wait_dollar` instead.
- A sentence terminated by CR/LF without a `*HH` suffix is delivered with `checksum_present == false`; some proprietary messages omit the checksum.
- The streaming parser does not reject an empty sentence type the way `decode_sentence()` does, matching real-device framing behavior; inspect `message_view::type` before use if that matters to the caller.
- All parser operations are single-threaded and non-reentrant; `feed()` must not be called recursively from within a callback.
