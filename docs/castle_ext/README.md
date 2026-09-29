# Castle Extensions

`castle_ext` is the home for header-only functionality that extends Castle beyond its general-purpose core into external standards, communication protocols, and domain-specific implementations. Its scope is intended to grow over time, with each area remaining independently usable so applications can include only what they need.

So far, `castle_ext` provides geodetic calculations and GNSS support for NMEA 0183 and u-blox UBX. These modules complement Castle's general-purpose containers, callbacks, math, and status utilities:

- **Geodetic** provides coordinate and datum types, conversions, distance and bearing algorithms, coordinate transformations, and geofences.
- **Parsers** consume NMEA or UBX byte streams, validate frames, and deliver protocol-specific message views. Optional decoder registries dispatch those views to typed message decoders.
- **Protocols** provides stateless framing, checksums, field readers, encoders, and typed NMEA sentence and UBX message models.

These boundaries let an application use only the pieces it needs. For example, a device driver can feed bytes to a parser and decode typed messages, while a separate application layer can use geodetic calculations without depending on a transport or parser. The same modular approach provides room to add support for other external standards, protocols, and reusable automotive features without coupling them to Castle's core.

## Modules

| Area | Capabilities | Documentation |
| --- | --- | --- |
| Geodetic | `geo_point`, DMS/DDM and angle conversions, WGS-84/PZ-90.11/Krasovsky datum policies, Haversine and Vincenty distance/bearing, WGS-84/GCJ-02 transforms, and fixed-capacity circular and polygon geofences. | [Coordinates](geodetic/coord.md), [datums](geodetic/datum.md), [distance and bearing](geodetic/distance.md), [transforms](geodetic/transform.md), [geofences](geodetic/geofence.md), [shared utilities](geodetic/geo_utils.md) |
| NMEA 0183 | Sentence framing and checksums, tokenized views, field parsing and encoding, typed sentence models, sentence generation, a streaming parser, and a protocol-bound decoder registry. | [Protocol primitives](protocols/nmea/nmea.md), [field cursor](protocols/nmea/field_cursor.md), [sentence models](protocols/nmea/messages/sentences.md), [generator](protocols/nmea/nmea_generator.md), [parser](parsers/nmea_parser/nmea_parser.md), [decoder registry](parsers/nmea_parser/decoder_registry.md) |
| u-blox UBX | Binary frame encoding/decoding, checksums and little-endian accessors, checked payload reading, VALSET/VALGET configuration helpers, typed messages, a streaming parser, and a protocol-bound decoder registry. | [Protocol primitives](protocols/ubx/ubx.md), [payload reader](protocols/ubx/payload_reader.md), [configuration](protocols/ubx/ubx_config.md), [message models](protocols/ubx/messages/messages.md), [parser](parsers/ubx_parser/ubx_parser.md), [decoder registry](parsers/ubx_parser/decoder_registry.md) |

All extensions are intended to fit embedded use: their protocol and parser implementations use fixed storage and Castle callbacks rather than requiring heap allocation, exceptions, RTTI, virtual dispatch, or STL containers. See each component page for its precise limits and lifetime requirements.
