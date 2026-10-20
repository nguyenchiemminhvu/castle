# UBX RXM-MEASX

## Overview

`rxm_measx<MaxSvs>` models UBX-RXM-MEASX (0x02 0x14), the periodic/polled satellite measurement output for RRLP: a 44-byte header followed by `numSV` 24-byte satellite blocks (`44 + numSV·24`). Payload data follows the RRLP (3GPP TS 44.031) conventions, except that GNSS/satellite IDs use the u-blox Satellite Numbering scheme.

Each block is an `rxm_measx_sv` holding the raw wire integers (Doppler, code phase, multipath indicator, RMS error index) plus helpers that apply the spec scales. The header keeps the four per-constellation reference times and their accuracies. Blocks are stored in a fixed-capacity [`castle::container::vector`](../../../../container/vector.md).

## Header

`#include "castle_ext/protocols/ubx/messages/rxm_measx.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/constants.hpp`](../../../../core/constants.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../container/vector.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/repeated_group.hpp`](detail/repeated_group.md)
- `stdint.h`

## Public API

All names live in `castle::protocols::ubx::messages`.

### Types

| Type | Description |
| --- | --- |
| `rxm_measx_mpath : uint8_t` | `enum class` for `mpathIndic` (RRLP multipath indicator): `not_measured = 0`, `low = 1`, `medium = 2`, `high = 3`. |
| `rxm_measx_sv` | One 24-byte satellite block. |
| `rxm_measx<MaxSvs = 64U>` | The message; `static_assert(MaxSvs <= 255)` because `numSV` is a U1 field. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `rxm_measx::msg_class` / `msg_id` | `UBX_CLASS_RXM` (`0x02`) / `UBX_ID_RXM_MEASX` (`0x14`) | Class/ID. |
| `rxm_measx::header_length` | `44U` | Header bytes. |
| `rxm_measx::block_length` / `rxm_measx_sv::wire_size` | `24U` | Bytes per satellite block. |
| `rxm_measx::FLAGS_TOW_SET` | `0x03U` (bits 1..0) | `towSet`: 0 = no, 1 or 2 = yes. |
| `rxm_measx::TOW_ACC_OVER_4S` | `0xFFFFU` | `*_tow_acc` value meaning "> 4 s". |

### `rxm_measx` fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | - | Message version (0x01 for this version). |
| `gps_tow` | `uint32_t` | ms | GPS measurement reference time. |
| `glo_tow` | `uint32_t` | ms | GLONASS measurement reference time. |
| `bds_tow` | `uint32_t` | ms | BeiDou measurement reference time. |
| `qzss_tow` | `uint32_t` | ms | QZSS measurement reference time. |
| `gps_tow_acc` | `uint16_t` | 2^-4 ms | GPS reference time accuracy. |
| `glo_tow_acc` | `uint16_t` | 2^-4 ms | GLONASS reference time accuracy. |
| `bds_tow_acc` | `uint16_t` | 2^-4 ms | BeiDou reference time accuracy. |
| `qzss_tow_acc` | `uint16_t` | 2^-4 ms | QZSS reference time accuracy. |
| `num_svs` | `uint8_t` | - | On-wire satellite count (`numSV`); may exceed `svs.size()`. |
| `flags` | `uint8_t` | - | Flags, see `FLAGS_*`. |
| `svs` | `castle::container::vector<rxm_measx_sv, MaxSvs>` | - | Stored satellite blocks; `svs.size()` is the stored count. |

Reserved header bytes (1..3, 16..19, 30..31, 36..43) are not modelled.

### `rxm_measx_sv` fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `gnss_id` | `uint8_t` | - | GNSS identifier (see Satellite Numbering). |
| `sv_id` | `uint8_t` | - | Satellite identifier (see Satellite Numbering). |
| `cno` | `uint8_t` | - | Carrier-to-noise ratio (0..63). |
| `mpath_indic` | `uint8_t` | - | Multipath indicator, see `rxm_measx_mpath`. |
| `doppler_ms` | `int32_t` | 0.04 m/s | Doppler measurement. |
| `doppler_hz` | `int32_t` | 0.2 Hz | Doppler measurement. |
| `whole_chips` | `uint16_t` | - | Whole value of the code phase (0..1022 for GPS). |
| `frac_chips` | `uint16_t` | - | Fractional value of the code phase (0..1023). |
| `code_phase` | `uint32_t` | 2^-21 ms | Code phase. |
| `int_code_phase` | `uint8_t` | ms | Integer part of the code phase. |
| `pseu_range_rms_err` | `uint8_t` | - | RRLP pseudorange RMS error index (0..63). |

The last two block bytes (22..23) are reserved.

### Functions

| Function | Description |
| --- | --- |
| `rxm_measx_mpath rxm_measx_sv::mpath() const` | `mpath_indic` as the enum (cast of the raw byte). |
| `double rxm_measx_sv::doppler_speed_mps() const` | `doppler_ms · 0.04` [m/s]. |
| `double rxm_measx_sv::doppler_frequency_hz() const` | `doppler_hz · 0.2` [Hz]. |
| `double rxm_measx_sv::code_phase_ms() const` | `code_phase · 2^-21` [ms] (scale exact in `double`). |
| `uint8_t rxm_measx::tow_set() const` | Raw `towSet` value, `flags & FLAGS_TOW_SET` (0 = not set, 1 or 2 = set). |
| `static constexpr double rxm_measx::tow_acc_ms(uint16_t raw_acc)` | Converts a raw `*_tow_acc` field to milliseconds (`raw_acc · 0.0625`). Does not special-case `TOW_ACC_OVER_4S`. |
| `static bool rxm_measx::matches(const message_view& raw)` | Class/ID check. |
| `static bool rxm_measx::decode(const message_view& raw, rxm_measx& out)` | Validates the exact length `44 + numSV·24`, decodes the header and up to `MaxSvs` blocks. |
| `void rxm_measx::encode(payload_writer& writer) const` | Writes the header (`numSV` from `svs.size()`) and every stored block; use with `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/rxm_measx.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    rxm_measx<16U> measx{};
    if (!rxm_measx<16U>::decode(raw, measx) || (measx.tow_set() == 0U))
    {
        return;
    }
    for (const rxm_measx_sv& sv : measx.svs)
    {
        if (sv.mpath() != rxm_measx_mpath::high)
        {
            const double range_rate = sv.doppler_speed_mps();
            const double phase = sv.code_phase_ms();
            // ...
        }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/rxm_measx.cpp` for encode, decode and every accessor.

## Constraints & Notes

- Length validation: `decode()` returns `false` with `out` value-initialised unless the class/ID match, the payload holds at least the 44-byte header, and the payload length is exactly `44 + numSV·24`.
- Capacity: storage is capped by `MaxSvs` (default 64). Blocks beyond capacity are skipped and `decode()` still succeeds; `num_svs` keeps the on-wire count, which may exceed the stored `svs.size()`.
- `encode()` writes `numSV` from `svs.size()`; the `num_svs` member is ignored when encoding. Reserved bytes are written as zero.
- Scaled values stay raw integers in the model; the `double` helpers apply the scale on demand. Decoding does no floating-point math.
- Storage is a `castle::container::vector`: `svs.size()` is the stored count and `num_svs` the on-wire count. The field names of the earlier model (`gnss_id`, `doppler_ms`, `gps_tow_acc`, `num_svs`, ...) are preserved.
- No heap allocation (the vector is embedded in the value), no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
