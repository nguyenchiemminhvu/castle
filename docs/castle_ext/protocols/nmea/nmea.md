# NMEA protocol

## Overview
`nmea.hpp` provides the deterministic, header-only building blocks for the NMEA 0183 text protocol: framing constants, well-known talker/sentence identifiers, XOR checksum calculation, hex encode/decode, a non-owning tokenized sentence view, caller-buffer sentence encode/decode, and generic textual field decoders (numbers, lat/lon, UTC time/date). It contains no parsing state machine; that lives in [`nmea_parser`](../../parsers/nmea_parser/nmea_parser.md).

## Header
`#include "castle_ext/protocols/nmea/nmea.hpp"`

## Dependencies
- [`core/compiler.hpp`](../../../core/compiler.md)
- [`core/types.hpp`](../../../core/types.md)
- [`error/status.hpp`](../../../error/status.md)
- [`container/array_view.hpp`](../../../container/array_view.md)
- [`container/string_view.hpp`](../../../container/string_view.md)

## Public API
| API | Description |
| --- | --- |
| `NMEA_START_CHAR`, `NMEA_CHECKSUM_CHAR`, `NMEA_FIELD_SEP`, `NMEA_CR`, `NMEA_LF` | Framing delimiter bytes (`$`, `*`, `,`, CR, LF). |
| `NMEA_MAX_SENTENCE_LEN`, `NMEA_SAFE_MAX_SENTENCE_LEN` | NMEA 0183 standard length (82) and a practical embedded default (256) that also covers oversized proprietary messages. |
| `NMEA_DEFAULT_MAX_FIELDS` | Default per-sentence field-table capacity (32). |
| `TALKER_GP/GL/GA/GB/GQ/GN/P` | Well-known talker ID string constants. |
| `SENTENCE_GGA/RMC/GSA/GSV/GLL/VTG/GBS/GNS/GST/ZDA/DTM/GRS/TXT` | Well-known sentence type string constants. |
| `message_view` | Non-owning tokenized sentence: `talker`, `type`, `fields` (`array_view<const string_view>`), `checksum`, `checksum_present`. Accessors: `field(i)`, `has_field(i)`, `field_count()`, `is(type)`, `is_talker(talker)`. |
| `checksum_accumulator` | Streaming XOR accumulator: `reset()`, `update(byte)`, `value()`. |
| `is_hex_digit(c)`, `hex_nibble(c)`, `parse_hex_byte(hi, lo, out)`, `write_hex_byte(value, out)` | Hex codec for the `*HH` checksum suffix. |
| `compute_checksum(data)` | XOR checksum over a raw character range. |
| `split_identifier(id, talker, type)` | Splits a combined `"GNGGA"`/`"PUBX"`/`"GGA"` identifier into talker and type. |
| `tokenize_fields(content, field_storage, field_capacity, talker, type)` | Splits sentence content into talker/type and up to `field_capacity` fields. |
| `decode_sentence(line, field_storage, field_capacity, output)` | Pure, stateless decode of one complete sentence line. Returns `bool`. |
| `encode_sentence(talker, type, fields, output, capacity, written)` | Encodes one complete sentence into a caller buffer. Returns `castle::status`. |
| `parse_double/int/uint(text, out[, base])`, `parse_char(text, out)` | Generic numeric/character field decoders. |
| `parse_latlon(value, direction, out)` | Converts `DDmm.mmmm`/`DDDmm.mmmm` + N/S/E/W into signed decimal degrees. |
| `parse_utc_time(text, out)`, `parse_utc_date(text, out)` | Raw `hhmmss.ss` / `ddmmyy` numeric decoders. |

## Usage Example
See `samples/castle_ext/protocols/nmea/nmea.cpp`.

```cpp
#include "castle_ext/protocols/nmea/nmea.hpp"

char sentence[96U];
castle::container::string_view fields_data[2U] = {
    castle::container::string_view("1"),
    castle::container::string_view("08")
};
castle::container::array_view<const castle::container::string_view> fields(fields_data, 2U);

castle::size_type written = 0U;
castle::protocols::nmea::encode_sentence(
    castle::container::string_view(castle::protocols::nmea::TALKER_GP),
    castle::container::string_view(castle::protocols::nmea::SENTENCE_GSA),
    fields, sentence, sizeof(sentence), written);

castle::container::string_view field_storage[8U];
castle::protocols::nmea::message_view view;
castle::protocols::nmea::decode_sentence(
    castle::container::string_view(sentence, written), field_storage, 8U, view);
```

## Constraints & Notes
- No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers.
- `message_view::talker`/`type`/`fields` alias the content buffer passed to `decode_sentence()` (or the parser's internal buffer when produced by `nmea_parser`); valid only while that storage is unchanged.
- `tokenize_fields()` silently drops fields beyond `field_capacity`; `field_count()` reflects only the stored subset.
- `decode_sentence()` is intentionally strict (rejects an empty sentence type) for standalone validation and unit testing; the streaming parser is more permissive to match real-device behavior.
- `parse_double/int/uint()` reject fields longer than `NMEA_MAX_NUMERIC_FIELD_LEN` (31 characters) rather than truncating them, to avoid silently decoding a wrong value.
- This header does not decode specific sentence types (e.g. GGA fix fields) into typed structs; combine `message_view::field(i)` with the generic `parse_*` helpers to decode specific messages.
