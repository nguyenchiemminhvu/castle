# UBX RXM-RAWX

## Overview

`rxm_rawx<MaxMeas>` models UBX-RXM-RAWX (0x02 0x15), the periodic/polled multi-GNSS raw measurement output used to generate RINEX 3 observations: a 16-byte header followed by `numMeas` 32-byte measurement blocks (`16 + numMeas·32`). Each block is an `rxm_rawx_meas` carrying pseudorange, carrier phase, Doppler and signal-quality data, plus accessors for the standard-deviation indices and the tracking status bits.

`rcvTow`, `prMes`, `cpMes` (R8) and `doMes` (R4) are IEEE-754 values on the wire and are stored as `double` / `float`. Blocks are stored in a fixed-capacity [`castle::container::vector`](../../../../container/vector.md).

## Header

`#include "castle_ext/protocols/ubx/messages/rxm_rawx.hpp"`

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
| `rxm_rawx_meas` | One 32-byte measurement block. |
| `rxm_rawx<MaxMeas = 64U>` | The message; `static_assert(MaxMeas <= 255)` because `numMeas` is a U1 field. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `rxm_rawx::msg_class` / `msg_id` | `UBX_CLASS_RXM` (`0x02`) / `UBX_ID_RXM_RAWX` (`0x15`) | Class/ID. |
| `rxm_rawx::header_length` | `16U` | Header bytes. |
| `rxm_rawx::block_length` / `rxm_rawx_meas::wire_size` | `32U` | Bytes per measurement block. |
| `rxm_rawx::REC_STAT_LEAP_SEC` | `0x01U` (bit 0) | `leapSec`: leap seconds determined. |
| `rxm_rawx::REC_STAT_CLK_RESET` | `0x02U` (bit 1) | `clkReset`: clock reset applied. |
| `rxm_rawx_meas::STDEV_INDEX` | `0x0FU` (bits 3..0) | `prStd` / `cpStd` / `doStd` inside `pr_stdev` / `cp_stdev` / `do_stdev`. |
| `rxm_rawx_meas::CP_STD_INVALID` | `0x0FU` | `cpStd` value marking the carrier phase standard deviation as invalid. |
| `rxm_rawx_meas::TRK_STAT_PR_VALID` | `0x01U` (bit 0) | `prValid`: pseudorange valid. |
| `rxm_rawx_meas::TRK_STAT_CP_VALID` | `0x02U` (bit 1) | `cpValid`: carrier phase valid. |
| `rxm_rawx_meas::TRK_STAT_HALF_CYC` | `0x04U` (bit 2) | `halfCyc`: half cycle valid. |
| `rxm_rawx_meas::TRK_STAT_SUB_HALF_CYC` | `0x08U` (bit 3) | `subHalfCyc`: half cycle subtracted from phase. |

### `rxm_rawx` fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `rcv_tow` | `double` | s | Measurement time of week in receiver local time (R8). |
| `week` | `uint16_t` | weeks | GPS week number in receiver local time. |
| `leap_s` | `int8_t` | s | GPS leap seconds (GPS-UTC). |
| `num_meas` | `uint8_t` | - | On-wire measurement count; may exceed `meas.size()`. |
| `rec_stat` | `uint8_t` | - | Receiver tracking status, see `REC_STAT_*`. |
| `version` | `uint8_t` | - | Message version (0x01 for this version). |
| `meas` | `castle::container::vector<rxm_rawx_meas, MaxMeas>` | - | Stored measurement blocks; `meas.size()` is the stored count. |

Header bytes 14..15 are reserved and not modelled.

### `rxm_rawx_meas` fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `pr_mes` | `double` | m | Pseudorange measurement (R8). |
| `cp_mes` | `double` | cycles | Carrier phase measurement (R8). |
| `do_mes` | `float` | Hz | Doppler measurement, positive for approaching satellites (R4). |
| `gnss_id` | `uint8_t` | - | GNSS identifier (see Satellite Numbering). |
| `sv_id` | `uint8_t` | - | Satellite identifier (see Satellite Numbering). |
| `sig_id` | `uint8_t` | - | Signal identifier (see Signal Identifiers). |
| `freq_id` | `uint8_t` | - | GLONASS frequency slot + 7 (0..13). |
| `locktime` | `uint16_t` | ms | Carrier phase locktime counter, at most 64500. |
| `cno` | `uint8_t` | dBHz | Carrier-to-noise density ratio. |
| `pr_stdev` | `uint8_t` | `0.01·2^n` m | Pseudorange standard deviation index (bits 3..0). |
| `cp_stdev` | `uint8_t` | `0.004·n` cycles | Carrier phase standard deviation (bits 3..0). |
| `do_stdev` | `uint8_t` | `0.002·2^n` Hz | Doppler standard deviation index (bits 3..0). |
| `trk_stat` | `uint8_t` | - | Tracking status, see `TRK_STAT_*`. |

The last block byte (31) is reserved.

### Functions

| Function | Description |
| --- | --- |
| `uint8_t rxm_rawx_meas::pr_std() const` | `pr_stdev & STDEV_INDEX`. |
| `uint8_t rxm_rawx_meas::cp_std() const` | `cp_stdev & STDEV_INDEX`. |
| `uint8_t rxm_rawx_meas::do_std() const` | `do_stdev & STDEV_INDEX`. |
| `double rxm_rawx_meas::pr_std_m() const` | Estimated pseudorange standard deviation: `0.01 · 2^prStd` [m]. |
| `double rxm_rawx_meas::cp_std_cycles() const` | Estimated carrier phase standard deviation: `0.004 · cpStd` [cycles]. Meaningless when `cp_std_valid()` is `false`. |
| `bool rxm_rawx_meas::cp_std_valid() const` | `false` when `cpStd` holds the invalid marker `CP_STD_INVALID` (0x0F). |
| `double rxm_rawx_meas::do_std_hz() const` | Estimated Doppler standard deviation: `0.002 · 2^doStd` [Hz]. |
| `bool rxm_rawx_meas::pr_valid() const` | `TRK_STAT_PR_VALID` set. |
| `bool rxm_rawx_meas::cp_valid() const` | `TRK_STAT_CP_VALID` set. |
| `bool rxm_rawx_meas::half_cyc() const` | `TRK_STAT_HALF_CYC` set. |
| `bool rxm_rawx_meas::sub_half_cyc() const` | `TRK_STAT_SUB_HALF_CYC` set. |
| `bool rxm_rawx::leap_sec() const` | `REC_STAT_LEAP_SEC` set. |
| `bool rxm_rawx::clk_reset() const` | `REC_STAT_CLK_RESET` set. |
| `static bool rxm_rawx::matches(const message_view& raw)` | Class/ID check. |
| `static bool rxm_rawx::decode(const message_view& raw, rxm_rawx& out)` | Validates the exact length `16 + numMeas·32`, decodes the header and up to `MaxMeas` blocks. |
| `void rxm_rawx::encode(payload_writer& writer) const` | Writes the header (`numMeas` from `meas.size()`) and every stored block; use with `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/rxm_rawx.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    rxm_rawx<32U> rawx{};
    if (!rxm_rawx<32U>::decode(raw, rawx) || !rawx.leap_sec())
    {
        return;
    }
    for (const rxm_rawx_meas& m : rawx.meas)
    {
        if (m.pr_valid() && m.cp_valid() && m.cp_std_valid())
        {
            const double range = m.pr_mes;          // [m]
            const double sigma = m.pr_std_m();      // [m]
            const double phase_sigma = m.cp_std_cycles();
            // ...
        }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/rxm_rawx.cpp` for encode, decode and every accessor.

## Constraints & Notes

- Length validation: `decode()` returns `false` with `out` value-initialised unless the class/ID match, the payload holds at least the 16-byte header, and the payload length is exactly `16 + numMeas·32`.
- Capacity: storage is capped by `MaxMeas` (default 64). Blocks beyond capacity are skipped and `decode()` still succeeds; `num_meas` keeps the on-wire count, which may exceed the stored `meas.size()`.
- `encode()` writes `numMeas` from `meas.size()`; the `num_meas` member is ignored when encoding. Reserved bytes are written as zero.
- R8/R4 fields (`rcv_tow`, `pr_mes`, `cp_mes`, `do_mes`) are transferred bit-exact: the reader/writer reinterpret the little-endian 64/32-bit pattern as `double` / `float` (`bit_cast`), with no conversion or rounding, so NaN payloads and signed zeros survive a round trip.
- The standard-deviation helpers compute `0.01·2^n` m, `0.004·n` cycles and `0.002·2^n` Hz on demand from the 4-bit indices; the raw bytes are kept unchanged (bits 7..4 are ignored by the helpers).
- No heap allocation (the vector is embedded in the value), no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
