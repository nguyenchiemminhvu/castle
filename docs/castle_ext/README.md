# Castle Extensions

`castle_ext` is the home for header-only functionality that extends Castle beyond its general-purpose core into external standards, communication protocols, and domain-specific implementations. Its scope is intended to grow over time, with each area remaining independently usable so applications can include only what they need.

So far, `castle_ext` provides geodetic calculations, GNSS support for NMEA 0183 and u-blox UBX, and a platform-neutral GPIO abstraction. These modules complement Castle's general-purpose containers, callbacks, math, and status utilities:

- **Geodetic** provides coordinate and datum types, conversions, distance and bearing algorithms, coordinate transformations, and geofences.
- **Checksums** provides table-free streaming and one-shot CRC-8/16/32/64, Fletcher-16/32/64, and XOR calculations.
- **Codecs** provides fixed-buffer Base32, Base64, and hexadecimal encoding and decoding with caller-owned storage.
- **Crypto** provides heap-free AES block and CTR encryption, ChaCha20 streaming encryption, and MD5, SHA-256, and SHA-512 hashing primitives.
- **Parsers** consume NMEA or UBX byte streams, validate frames, and deliver protocol-specific message views. Optional decoder registries dispatch those views to typed message decoders.
- **Protocols** provides stateless framing, checksums, field readers, encoders, and typed NMEA sentence and UBX message models.
- **GPIO** provides a backend-independent pin/port facade over platform adapters (mock, Linux value-file, Zephyr device tree), with no heap, virtual dispatch, RTTI, or STL dependency.
- **Timing** provides a transport-neutral IEEE 1588 PTPv2 slave prototype for fixed-storage Sync, Follow_Up, Delay_Req, and Delay_Resp exchanges.

These boundaries let an application use only the pieces it needs. For example, a device driver can feed bytes to a parser and decode typed messages, while a separate application layer can use geodetic calculations without depending on a transport or parser. The same modular approach provides room to add support for other external standards, protocols, and reusable automotive features without coupling them to Castle's core.

## Modules

| Area | Capabilities | Documentation |
| --- | --- | --- |
| Checksums | Table-free streaming and one-shot CRC-8/16/32/64, Fletcher-16/32/64, and XOR checksums for deterministic, non-cryptographic error detection. | [CRC-8](checksums/crc8.md), [CRC-16](checksums/crc16.md), [CRC-32](checksums/crc32.md), [CRC-64](checksums/crc64.md), [Fletcher-16](checksums/fletcher16.md), [Fletcher-32](checksums/fletcher32.md), [Fletcher-64](checksums/fletcher64.md), [Fletcher engine](checksums/fletcher_common.md), [XOR](checksums/xor.md) |
| Codecs | Caller-buffered Base32 and Base64 codecs, including padded or unpadded forms and standard or URL-safe Base64, plus hexadecimal encoding and decoding. | [Base32](codecs/base32.md), [Base64](codecs/base64.md), [hexadecimal](codecs/hex.md) |
| Crypto | Fixed-size AES-128/192/256 block and CTR operations, RFC 8439 ChaCha20, and streaming MD5, SHA-256, and SHA-512 hash primitives. | [AES](crypto/aes.md), [ChaCha20](crypto/chacha20.md), [MD5](crypto/hash/md5.md), [SHA-256](crypto/hash/sha256.md), [SHA-512](crypto/hash/sha512.md) |
| Geodetic | `geo_point`, DMS/DDM and angle conversions, WGS-84/PZ-90.11/Krasovsky datum policies, Haversine and Vincenty distance/bearing, WGS-84/GCJ-02 transforms, and fixed-capacity circular and polygon geofences. | [Coordinates](geodetic/coord.md), [datums](geodetic/datum.md), [distance and bearing](geodetic/distance.md), [transforms](geodetic/transform.md), [geofences](geodetic/geofence.md), [shared utilities](geodetic/geo_utils.md) |
| NMEA 0183 | Sentence framing and checksums, tokenized views, field parsing and encoding, typed sentence models, sentence generation, a streaming parser, and a protocol-bound decoder registry. | [Protocol primitives](protocols/nmea/nmea.md), [field cursor](protocols/nmea/field_cursor.md), [sentence models](protocols/nmea/messages/sentences.md), [generator](protocols/nmea/nmea_generator.md), [parser](parsers/nmea_parser/nmea_parser.md), [decoder registry](parsers/nmea_parser/decoder_registry.md) |
| u-blox UBX | Binary frame encoding/decoding, checksums and little-endian accessors, checked payload reading, VALSET/VALGET configuration helpers, typed messages, a streaming parser, and a protocol-bound decoder registry. | [Protocol primitives](protocols/ubx/ubx.md), [payload reader](protocols/ubx/payload_reader.md), [configuration](protocols/ubx/ubx_config.md), [message models](protocols/ubx/messages/messages.md), [parser](parsers/ubx_parser/ubx_parser.md), [decoder registry](parsers/ubx_parser/decoder_registry.md) |
| GPIO | Platform-neutral pin/port facades (`basic_pin`, `basic_port`) over a compile-time-checked backend contract, with mock, Linux value-file, and Zephyr device-tree backends. | [Types](gpio/types.md), [pin](gpio/pin.md), [port](gpio/port.md), [backends](gpio/backends.md) |
| Timing / PTP | IEEE 1588-2008 PTPv2 slave exchange with one-step and two-step Sync support, fixed-buffer Delay_Req encoding, timestamp correlation, and clock-adjustment callback integration. | [PTP slave prototype](protocols/timing/ptp_v2.md) |

All extensions are intended to fit embedded use: their protocol and parser implementations use fixed storage and Castle callbacks rather than requiring heap allocation, exceptions, RTTI, virtual dispatch, or STL containers. See each component page for its precise limits and lifetime requirements.
