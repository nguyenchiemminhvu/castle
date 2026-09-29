# NMEA generator

## Overview
`nmea_generator.hpp` provides deterministic, header-only generation of NMEA 0183 messages from strongly-typed input structures. It builds complete `$talker+type,fields...*HH\r\n` messages into caller-provided buffers without dynamic allocation and reuses the protocol primitives from [`nmea`](../nmea/nmea.md) for checksumon and framing constants.

The module provides:
- Plain input structures describing one sentence instance (`gga_input`, `rmc_input`, `gsa_input`, `gsv_input`, `vtg_input`, `gll_input`, `zda_input`);
- One `generate_xxx()` free function per supported sentence type;
- Helper functions for UTC time/date formatting, latitude/longitude formatting, checksum finalization, and GSV sentence counting;
- Deterministic sentence generation using `castle::string_builder`.

Unlike `nmea::encode_sentence()`, this generator can emit conditionally empty fields required by NMEA sentence specifications (for example HDOP, magnetic variation, DGPS fields, SNR fields, etc.) while still producing valid checksummed messages.

## Header
`#include "castle_ext/protocols/nmea/nmea_generator.hpp"`

## Dependencies
- ../../../core/compiler.md
- ../../../core/types.md
- ../../../error/status.md
- ../../../container/array_view.md
- ../../../container/string_view.md
- ../../../utility/string_builder.md
- ../nmea/nmea.md

## Public API

| API | Description |
| --- | --- |
| `NMEA_GENERATOR_MAX_SENTENCE_LEN` | Default scratch-buffer capacity used while assembling one sentence. Defaults to `NMEA_SAFE_MAX_SENTENCE_LEN`. |
| `NMEA_GSV_SATS_PER_MESSAGE` | Maximum satellite records carried by one `$--GSV` sentence (4). |
| `gga_input` | Input structure for a `$--GGA` sentence. |
| `rmc_input` | Input structure for a `$--RMC` sentence. |
| `gsa_input` | Input structure for a `$--GSA` sentence. |
| `gsv_satellite_record` | One satellite record contained within a `gsv_input`. |
| `gsv_input` | Input structure representing a complete `$--GSV` message set. |
| `vtg_input` | Input structure for a `$--VTG` sentence. |
| `gll_input` | Input structure for a `$--GLL` sentence. |
| `zda_input` | Input structure for a `$--ZDA` sentence. |
| `append_padded(builder, value, width)` | Appends a zero-padded numeric value. |
| `append_utc_time(...)` | Formats UTC time as `hhmmss.ss`. |
| `append_date(...)` | Formats date as `ddmmyy`. |
| `append_latitude(...)` | Formats latitude magnitude as `DDmm.mmmmm`. |
| `append_longitude(...)` | Formats longitude magnitude as `DDDmm.mmmmm`. |
| `ns_indicator(latitude)` | Returns `'N'` or `'S'`. |
| `ew_indicator(longitude)` | Returns `'E'` or `'W'`. |
| `finalize_sentence(...)` | Computes checksum, appends `*HH\r\n`, validates capacity, and copies to caller output buffer. |
| `generate_gga(...)` | Generates one `$--GGA` sentence. |
| `generate_rmc(...)` | Generates one `$--RMC` sentence. |
| `generate_gsa(...)` | Generates one `$--GSA` sentence. |
| `generate_vtg(...)` | Generates one `$--VTG` sentence. |
| `generate_gll(...)` | Generates one `$--GLL` sentence. |
| `generate_zda(...)` | Generates one `$--ZDA` sentence. |
| `gsv_message_count(input)` | Returns the number of `$--GSV` messages required for all satellites. |
| `generate_gsv_message(...)` | Generates one sentence from a multi-sentence `$--GSV` set. |

## Input Structures

### `gga_input`
Represents a Global Positioning System Fix Data (`GGA`) sentence.

Fields include:
- UTC time (`hour`, `minute`, `second`, `millisecond`)
- Latitude/longitude in decimal degrees
- Fix quality
- Satellite count
- HDOP
- MSL altitude
- Geoid separation
- Optional DGPS age/station ID
- `valid` flag

### `rmc_input`
Represents a Recommended Minimum Specific GNSS Data (`RMC`) sentence.

Fields include:
- UTC time/date
- Active/void status
- Position
- Speed over ground
- Course over ground
- Optional magnetic variation
- NMEA mode indicator
- `valid` flag

### `gsa_input`
Represents a GNSS DOP and Active Satellites (`GSA`) sentence.

Fields include:
- Operating mode (`A`/`M`)
- Navigation mode (1/2/3)
- Up to 12 satellite IDs
- PDOP/HDOP/VDOP
- Optional NMEA 4.11 system identifier
- `valid` flag

### `gsv_satellite_record`
Represents one satellite entry within a `GSV` message.

Fields include:
- Satellite ID
- Elevation
- Azimuth
- SNR

Negative elevation values and zero SNR values generate empty NMEA fields.

### `gsv_input`
Represents an entire constellation's satellite list.

Fields include:
- `array_view<const gsv_satellite_record> satellites`
- `valid` flag

Multiple NMEA `GSV` messages may be required to transmit all satellites.

### `vtg_input`
Represents a Course Over Ground and Ground Speed (`VTG`) sentence.

Fields include:
- True course
- Optional magnetic course
- Speed in knots
- Speed in km/h
- Mode indicator
- `valid` flag

### `gll_input`
Represents a Geographic Position Latitude/Longitude (`GLL`) sentence.

Fields include:
- Position
- UTC time
- Active/void status
- Mode indicator
- `valid` flag

### `zda_input`
Represents a Time and Date (`ZDA`) sentence.

Fields include:
- UTC time
- Day/month/year
- Local timezone hours/minutes
- `valid` flag

## Usage Example

See `samples/castle_ext/protocols/nmea/nmea_generator.cpp`.

```cpp
#include "castle_ext/protocols/nmea/nmea_generator.hpp"

using namespace castle::protocols::nmea::generator;

gga_input input;
input.hour = 9;
input.minute = 27;
input.second = 25;
input.millisecond = 0;

input.latitude_deg = 47.2852333;
input.longitude_deg = 8.5652650;

input.fix_quality = 1;
input.num_satellites = 8;
input.hdop = 0.9;
input.altitude_msl_m = 499.6;
input.geoid_sep_m = 48.0;
input.valid = true;

char sentence[128U];
castle::size_type written = 0U;

generate_gga(
    input,
    castle::container::string_view(castle::protocols::nmea::TALKER_GP),
    sentence,
    sizeof(sentence),
    written);
```

## Generating an RMC Sentence

```cpp
using namespace castle::protocols::nmea::generator;

rmc_input input;
input.hour = 9;
input.minute = 27;
input.second = 25;

input.day = 12;
input.month = 8;
input.year = 2026;

input.status_active = true;
input.latitude_deg = 47.2852333;
input.longitude_deg = 8.5652650;

input.speed_knots = 3.25;
input.course_true = 182.5;

input.mode = 'A';
input.valid = true;

char sentence[128U];
castle::size_type written = 0U;

generate_rmc(
    input,
    castle::container::string_view(castle::protocols::nmea::TALKER_GP),
    sentence,
    sizeof(sentence),
    written);
```

## Generating a GSV Message Set

```cpp
using namespace castle::protocols::nmea::generator;

gsv_satellite_record satellites[5U];

satellites[0U].sv_id = 1U;
satellites[0U].elevation_deg = 45;
satellites[0U].azimuth_deg = 120;
satellites[0U].snr = 38U;

satellites[1U].sv_id = 3U;
satellites[2U].sv_id = 5U;
satellites[3U].sv_id = 8U;
satellites[4U].sv_id = 12U;

gsv_input input;
input.satellites =
    castle::container::array_view<const gsv_satellite_record>(satellites, 5U);
input.valid = true;

const castle::size_type count = gsv_message_count(input);

for (castle::size_type message = 1U; message <= count; ++message)
{
    char sentence[128U];
    castle::size_type written = 0U;

    generate_gsv_message(
        input,
        castle::container::string_view(castle::protocols::nmea::TALKER_GP),
        message,
        sentence,
        sizeof(sentence),
        written);
}
```

## Status Codes

All sentence generation APIs return `castle::status`.

| Status | Meaning |
| --- | --- |
| `castle::status::ok` | Sentence generated successfully. |
| `castle::status::invalid_argument` | Invalid input structure, invalid message number, or `output == nullptr`. |
| `castle::status::full` | Generated sentence exceeded scratch-buffer or output-buffer capacity. |

## Constraints & Notes

- No heap allocation, exceptions, RTTI, virtual dispatch, or STL containers.
- All APIs operate entirely on caller-owned storage.
- Sentence checksums are calculated using `nmea::compute_checksum()`.
- Hex checksum formatting reuses `nmea::write_hex_byte()`.
- The final output format is always:

```text
$<talker><type>,<fields...>*HH\r\n
```

- `generate_gga()`, *generate_rmc()`, `generate_gsa()`,*`generate_vtg()`, `generate_gll()`* and `generate_zda()` generate exa*tly one complete sentence per call*
- `generate_gsv_message()` genera*es one sentence from a potentially*multi-sentence GSV set. Use `gsv_m*ssage_count()` to determine how ma*y messages must be emitted.
- Seve*al sentence types intentionally em*t empty fields when optional infor*ation is unavailable rather than i*serting placeholder numeric values*
- The `valid` flag in every input*structure provides a simple determ*nistic guard against generating in*omplete or uninitialized protocol *ata.
- Output is never null-termin*ted automatically; callers should *se `bytes_written` to determine th* valid sentence length.
- The scra*ch buffer size is controlled by th* `MaxSentenceLen` template parameter and defaults to `NMEA_GENERATOR_MAX_SENTENCE_LEN`.
- All generator functions are fully header-only and suitable for embedded systems requiring deterministic runtime behavior.
