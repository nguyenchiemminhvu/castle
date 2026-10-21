# RTCM MSM4 / MSM5 / MSM7

## Overview

`msm.hpp` provides `msm_message`, the encode-only model of the RTCM 3.3 **Multiple Signal Messages** MSM4, MSM5 and MSM7, the current observation format that a u-blox ZED-F9R accepts as correction input. One message carries all satellites and signals of one epoch of one constellation. The module only **builds** frames that are sent to the receiver; there is no decoder or parser by design.

| System | MSM4 | MSM5 | MSM7 | Aliases |
| --- | --- | --- | --- | --- |
| GPS | 1074 | 1075 | 1077 | `gps_msm4`, `gps_msm5`, `gps_msm7` |
| GLONASS | 1084 | 1085 | 1087 | `glonass_msm4`, `glonass_msm5`, `glonass_msm7` |
| Galileo | 1094 | 1095 | 1097 | `galileo_msm4`, `galileo_msm5`, `galileo_msm7` |
| BeiDou | 1124 | 1125 | 1127 | `beidou_msm4`, `beidou_msm5`, `beidou_msm7` |
| SBAS / QZSS | 1104 / 1114 | 1105 / 1115 | 1107 / 1117 | none; instantiate `msm_message<msm_system::sbas, 4U>` etc. |

The message number (DF002) is `static_cast<uint16_t>(System) + Level`, where `msm_system` holds the MSM1 number minus one (`gps = 1070`, `glonass = 1080`, `galileo = 1090`, `sbas = 1100`, `qzss = 1110`, `beidou = 1120`).

### Message structure

Every MSM payload (RTCM 10403.3 table 3.5-77 onwards) is a bit stream, MSB first, in this order:

1. **Header** (169 bits): DF002 message number (12), DF003 reference station ID (12), epoch time (30), DF393 multiple-message bit (1), DF409 IODS (3), DF001 reserved (7, written as zero), DF411 clock steering (2), DF412 external clock (2), DF417 divergence-free smoothing (1), DF418 smoothing interval (3), DF394 satellite mask (64), DF395 signal mask (32).
2. **Cell mask** DF396: `popcount(satellite_mask) * popcount(signal_mask)` bits (0..64).
3. **Satellite data**, field-major: DF397 for all satellites, then (MSM5/7) the extended satellite information of all satellites, then DF398 for all satellites, then (MSM5/7) DF399 for all satellites.
4. **Signal (cell) data**, field-major, one entry per set cell-mask bit: fine pseudorange (DF400 / DF405), fine phase-range (DF401 / DF406), lock time indicator (DF402 / DF407), half-cycle ambiguity DF420, CNR (DF403 / DF408), then (MSM5/7) fine phase-range rate DF404.

The epoch time field (30 bits) is DF004 GPS time of week in ms for GPS, DF248 for Galileo and DF427 for BeiDou. For GLONASS it is DF416 day of week (3 bits, 0 = Sunday, 7 = unknown) followed by DF034 time of day in ms (27 bits); use `set_glonass_epoch()`.

"Field-major" means each field is written for every satellite (or cell) before the next field starts, not entry by entry. The last byte is zero-padded.

### The three masks

* **Satellite mask (DF394, 64 bits)**: satellite ID `n` (1..64) is the bit `1 << (64 - n)`, so ID 1 is the **MSB**. Use `satellite_bit(n)`.
* **Signal mask (DF395, 32 bits)**: signal ID `n` (1..32) is the bit `1 << (32 - n)`, so ID 1 is the MSB. Use `signal_bit(n)`. Signal IDs are the RTCM per-constellation signal numbers (for GPS, 2 = L1 C/A, 16 = L2C(L), ...).
* **Cell mask (DF396, `nsat * nsig` bits)**: one bit per (satellite, signal) pair, **satellite-major** and MSB first. With `nsat` satellites and `nsig` signals, the cell of the satellite at index `s` (position among the set satellite-mask bits, 0-based) and the signal at index `g` is the bit `1 << (nsat * nsig - 1 - (s * nsig + g))`. Use `cell_bit(s, g, nsat, nsig)`. A set bit means the satellite is tracked on that signal; one `msm_signal` entry per set bit follows, in the same order (satellite 0 signals first). Because the product is at most 64, the cell mask fits `uint64_t`.

### Per-level field widths

| Field | DF | MSM4 | MSM5 | MSM7 | Unit / step | Invalid marker |
| --- | --- | --- | --- | --- | --- | --- |
| Rough range, integer ms | DF397 | 8 | 8 | 8 | 1 ms | 255 |
| Extended satellite info | - | - | 4 | 4 | GLONASS FCN + 7, else reserved | - |
| Rough range, mod 1 ms | DF398 | 10 | 10 | 10 | 2^-10 ms | - |
| Rough phase-range rate | DF399 | - | 14 (signed) | 14 (signed) | 1 m/s | -8192 |
| Fine pseudorange | DF400 / DF405 | 15 (signed) | 15 (signed) | 20 (signed) | MSM4/5: 2^-24 ms, MSM7: 2^-29 ms | -2^14 (MSM4/5), -2^19 (MSM7) |
| Fine phase-range | DF401 / DF406 | 22 (signed) | 22 (signed) | 24 (signed) | MSM4/5: 2^-29 ms, MSM7: 2^-31 ms | -2^21 (MSM4/5), -2^23 (MSM7) |
| Lock time indicator | DF402 / DF407 | 4 | 4 | 10 | RTCM lock-time code | - |
| Half-cycle ambiguity | DF420 | 1 | 1 | 1 | flag | - |
| CNR | DF403 / DF408 | 6 | 6 | 10 | MSM4/5: 1 dB-Hz, MSM7: 2^-4 dB-Hz | - |
| Fine phase-range rate | DF404 | - | 15 (signed) | 15 (signed) | 0.0001 m/s | -2^14 |

All millisecond values are **light-milliseconds**: a range in metres divided by 299792.458. The model stores the raw wire integers; the `set_*` helpers do the quantisation.

Payload size in bits: `169 + nsat*nsig + nsat*S + ncell*C`, where `S` is 18 (MSM4) or 36 (MSM5/7) and `C` is 48 (MSM4), 63 (MSM5) or 80 (MSM7). The largest MSM7 message (64 satellites x 1 signal) is 958 payload bytes, below the RTCM limit of 1023.

## Header

`#include "castle_ext/protocols/rtcm/messages/msm.hpp"`

## Dependencies

* [`bit/bit_count.hpp`](../../../../bit/bit_count.md)
* [`core/compiler.hpp`](../../../../core/compiler.md)
* [`core/types.hpp`](../../../../core/types.md)
* [`container/vector.hpp`](../../../../container/vector.md)
* [`error/status.hpp`](../../../../error/status.md)
* [`bit_writer.hpp`](../bit_writer.md)
* [`rtcm.hpp`](../rtcm.md)
* [`math/round.hpp`](../../../../math/round.md)
* `stdint.h`

## Public API

All names live in `castle::protocols::rtcm::messages`.

### Types

| Type | Description |
| --- | --- |
| `msm_system : uint16_t` | `enum class`: `gps = 1070`, `glonass = 1080`, `galileo = 1090`, `sbas = 1100`, `qzss = 1110`, `beidou = 1120` (MSM1 message number minus one). |
| `msm_satellite` | Per-satellite wire values (below). |
| `msm_signal` | Per-cell wire values (below). |
| `msm_message<System, Level, MaxSatellites = 64, MaxCells = 64>` | The message. `Level` must be 4, 5 or 7; `MaxSatellites` and `MaxCells` must be in `1..64` (all enforced by `static_assert`). |
| `gps_msm4/5/7`, `glonass_msm4/5/7`, `galileo_msm4/5/7`, `beidou_msm4/5/7` | Aliases of `msm_message<system, level>` with full capacity. |

### `msm_satellite`

| Field | Type | DF / width | Description |
| --- | --- | --- | --- |
| `rough_range_ms` | `uint8_t` | DF397, 8 | Integer milliseconds of the rough range; `255` = invalid. |
| `extended_info` | `uint8_t` | 4 (MSM5/7) | GLONASS frequency channel number + 7; reserved (0) for other systems. Ignored by MSM4. |
| `rough_range_mod` | `uint16_t` | DF398, 10 | Rough range modulo 1 ms in 2^-10 ms. |
| `rough_phase_range_rate` | `int16_t` | DF399, 14 signed (MSM5/7) | Rough phase-range rate in m/s; `-8192` = invalid. Ignored by MSM4. |

### `msm_signal`

| Field | Type | DF / width | Description |
| --- | --- | --- | --- |
| `fine_pseudorange` | `int32_t` | DF400 15 (MSM4/5), DF405 20 (MSM7), signed | Residual to the rough range. |
| `fine_phase_range` | `int32_t` | DF401 22 (MSM4/5), DF406 24 (MSM7), signed | Residual to the rough range. |
| `lock_time` | `uint16_t` | DF402 4 (MSM4/5), DF407 10 (MSM7) | Lock time indicator code (stored raw, no conversion from seconds). |
| `half_cycle` | `bool` | DF420, 1 | Half-cycle ambiguity present. |
| `cnr` | `uint16_t` | DF403 6 (MSM4/5), DF408 10 (MSM7) | Carrier-to-noise ratio. |
| `fine_phase_range_rate` | `int16_t` | DF404, 15 signed (MSM5/7) | Residual to the rough rate in 0.0001 m/s; `-16384` = invalid. Ignored by MSM4. |

### `msm_message` data members

| Member | Type | DF / width | Description |
| --- | --- | --- | --- |
| `station_id` | `uint16_t` | DF003, 12 | Reference station ID, `0..4095`. |
| `epoch_time` | `uint32_t` | 30 | GPS/Galileo/BeiDou time of week in ms, or the GLONASS day + time of day (see `set_glonass_epoch()`). |
| `multiple_message` | `bool` | DF393, 1 | More MSM messages follow for this epoch. |
| `iods` | `uint8_t` | DF409, 3 | Issue of data station. |
| `clock_steering` | `uint8_t` | DF411, 2 | 0 unknown, 1 not applied, 2 applied, 3 reserved. |
| `external_clock` | `uint8_t` | DF412, 2 | External clock indicator. |
| `divergence_free` | `bool` | DF417, 1 | Divergence-free smoothing indicator. |
| `smoothing_interval` | `uint8_t` | DF418, 3 | Smoothing interval. |
| `satellite_mask` | `uint64_t` | DF394, 64 | See `satellite_bit()`. |
| `signal_mask` | `uint32_t` | DF395, 32 | See `signal_bit()`. |
| `cell_mask` | `uint64_t` | DF396, `nsat * nsig` | See `cell_bit()`. |
| `satellites` | `castle::container::vector<msm_satellite, MaxSatellites>` | - | One entry per set bit of `satellite_mask`, in ascending ID order. |
| `signals` | `castle::container::vector<msm_signal, MaxCells>` | - | One entry per set bit of `cell_mask`, in cell order. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `MESSAGE_TYPE` | `uint16_t`, `System + Level` | DF002. |
| `MAX_CELLS` | `64` | Largest `nsat * nsig`. |
| `HAS_RATE` | `Level >= 5` | MSM5/7 carry extended info and the rate fields. |
| `PSEUDORANGE_BITS` | 15 (MSM4/5), 20 (MSM7) | DF400 / DF405 width. |
| `PHASE_RANGE_BITS` | 22 (MSM4/5), 24 (MSM7) | DF401 / DF406 width. |
| `LOCK_TIME_BITS` | 4 (MSM4/5), 10 (MSM7) | DF402 / DF407 width. |
| `CNR_BITS` | 6 (MSM4/5), 10 (MSM7) | DF403 / DF408 width. |
| `PSEUDORANGE_STEPS_PER_MS` | 2^24 (MSM4/5), 2^29 (MSM7) | Fine pseudorange LSBs per light-ms. |
| `PHASE_RANGE_STEPS_PER_MS` | 2^29 (MSM4/5), 2^31 (MSM7) | Fine phase-range LSBs per light-ms. |
| `CNR_STEPS_PER_DBHZ` | 1 (MSM4/5), 16 (MSM7) | CNR LSBs per dB-Hz. |
| `ROUGH_RANGE_STEPS_PER_MS` | 1024 | DF398 LSBs per ms. |
| `FINE_RATE_STEPS_PER_M_S` | 10000 | DF404 LSBs per m/s. |
| `INVALID_ROUGH_RANGE` | 255 | DF397 marker. |
| `INVALID_ROUGH_RATE` | -8192 | DF399 marker. |
| `INVALID_FINE_RATE` | -16384 | DF404 marker. |
| `INVALID_FINE_PSEUDORANGE` | -16384 (MSM4/5), -524288 (MSM7) | DF400 / DF405 marker. |
| `INVALID_FINE_PHASE_RANGE` | -2097152 (MSM4/5), -8388608 (MSM7) | DF401 / DF406 marker. |

### Functions

All are `static` unless noted, `noexcept`, allocation-free and O(1) except `encode()`.

| Function | Description |
| --- | --- |
| `bool accepts(uint16_t type)` | `type == MESSAGE_TYPE`. |
| `uint64_t satellite_bit(size_type id)` | Satellite-mask bit of ID `1..64` (`1 << (64 - id)`); `0` for any other ID. |
| `uint32_t signal_bit(size_type id)` | Signal-mask bit of ID `1..32` (`1 << (32 - id)`); `0` for any other ID. |
| `uint64_t cell_bit(size_type satellite_index, size_type signal_index, size_type satellite_count, size_type signal_count)` | Cell-mask bit for the cell; `0` when `satellite_index >= satellite_count`, `signal_index >= signal_count` or `satellite_count * signal_count > 64`. OR the results together to build `cell_mask`. |
| `void set_glonass_epoch(uint8_t day_of_week, uint32_t time_of_day_ms)` | *Member.* `epoch_time = (day_of_week & 7) << 27 \| (time_of_day_ms & 0x07FFFFFF)`; overflow bits of either argument are masked and never leak into the other field. |
| `void set_rough_range(msm_satellite& sat, double range_ms)` | Rounds the range (light-ms, `0 <= range_ms < 255`) to 2^-10 ms into `rough_range_ms` / `rough_range_mod`. Negative, NaN, `>= 255` and a value that rounds up to 255 set `rough_range_ms = 255`, `rough_range_mod = 0`. A fraction that rounds to 1024 carries into the integer ms. Leaves the rate untouched. |
| `void set_rough_range(msm_satellite& sat, double range_ms, double rate_m_s)` | As above and sets `rough_phase_range_rate` to the rate rounded half away from zero; `rate_m_s` outside `(-8191.5, 8191.5)` (or NaN) stores `-8192`. `static_assert`s `HAS_RATE`, so it does not compile for MSM4. |
| `void set_pseudorange(const msm_satellite& sat, msm_signal& cell, double range_ms)` | `cell.fine_pseudorange` = `range_ms` minus the **quantised** rough range of `sat` (`rough_range_ms + rough_range_mod / 1024`), in `PSEUDORANGE_STEPS_PER_MS`, rounded half away from zero. A residual that does not fit the signed field, or that rounds onto the marker, stores `INVALID_FINE_PSEUDORANGE`. |
| `void set_phase_range(const msm_satellite& sat, msm_signal& cell, double range_ms)` | Same for `fine_phase_range` with `PHASE_RANGE_STEPS_PER_MS` and `INVALID_FINE_PHASE_RANGE`. |
| `void set_phase_range_rate(const msm_satellite& sat, msm_signal& cell, double rate_m_s)` | `fine_phase_range_rate` = `rate_m_s - rough_phase_range_rate` in 0.0001 m/s; out-of-range stores `INVALID_FINE_RATE`. `static_assert`s `HAS_RATE`. |
| `void set_cnr(msm_signal& cell, double dbhz)` | Rounds to `CNR_STEPS_PER_DBHZ`, clamps negative values and NaN to 0 and saturates at `2^CNR_BITS - 1` (63 or 1023). |
| `void encode(bit_writer& writer) const` | Writes the payload, see below. Used by `encode_message()`. |

Always call `set_rough_range()` **before** `set_pseudorange()` / `set_phase_range()` / `set_phase_range_rate()` for the same satellite: the fine values are relative to what is stored in `sat`.

### Encoding

```cpp
castle::status encode_message(const Message&, uint8_t* output, size_type capacity, size_type& bytes_written);
```

`encode_message()` from [`bit_writer.hpp`](../bit_writer.md) builds the complete frame (`D3`, length, payload, CRC-24Q). `msm_message::encode()` first validates, then writes the fields in the order given in the overview.

| Condition | Status |
| --- | --- |
| `popcount(satellite_mask) * popcount(signal_mask) > 64` | `invalid_argument` |
| `satellites.size() != popcount(satellite_mask)` | `invalid_argument` |
| `signals.size() != popcount(cell_mask)` | `invalid_argument` |
| `cell_mask` has a bit above the `nsat * nsig` cell bits (also any bit when `nsat * nsig == 0`) | `out_of_range` |
| Any other field exceeds its wire width (`station_id > 4095`, `epoch_time >= 2^30`, `iods > 7`, `clock_steering > 3`, `external_clock > 3`, `smoothing_interval > 7`, `rough_range_mod > 1023`, `extended_info > 15`, `rough_phase_range_rate` outside `-8192..8191`, a fine value outside its signed range, `lock_time`/`cnr` above their width, `fine_phase_range_rate` outside `-16384..16383`) | `out_of_range` |
| `output == nullptr` | `invalid_argument` |
| Frame does not fit `capacity` (exact fit succeeds, one byte short fails) | `full` |

On any failure `bytes_written` is `0`. An empty message (no satellites, no signals) is valid and encodes the 169-bit header (22 payload bytes). MSM4 ignores `extended_info`, `rough_phase_range_rate` and `fine_phase_range_rate`.

## Usage Example

```cpp
#include "castle_ext/protocols/rtcm/messages/msm.hpp"

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

bool build_epoch(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    gps_msm7 epoch{};
    epoch.station_id = 1234U;
    epoch.epoch_time = 345601000U;                       // GPS time of week, ms

    // Two satellites (IDs 5 and 12) on two signals (IDs 2 and 16): four cells, all present.
    epoch.satellite_mask = gps_msm7::satellite_bit(5U) | gps_msm7::satellite_bit(12U);
    epoch.signal_mask = gps_msm7::signal_bit(2U) | gps_msm7::signal_bit(16U);
    for (castle::size_type sat = 0U; sat < 2U; ++sat)
    {
        for (castle::size_type sig = 0U; sig < 2U; ++sig)
        {
            epoch.cell_mask |= gps_msm7::cell_bit(sat, sig, 2U, 2U);
        }
    }

    const double range_ms[2] = {71.2, 77.0};            // metres / 299792.458
    const double rate_m_s[2] = {-412.35, 120.5};
    for (castle::size_type sat_index = 0U; sat_index < 2U; ++sat_index)
    {
        msm_satellite sat{};
        gps_msm7::set_rough_range(sat, range_ms[sat_index], rate_m_s[sat_index]);
        static_cast<void>(epoch.satellites.push_back(sat));

        for (castle::size_type sig = 0U; sig < 2U; ++sig)
        {
            msm_signal cell{};
            gps_msm7::set_pseudorange(sat, cell, range_ms[sat_index] + 0.00001 * sig);
            gps_msm7::set_phase_range(sat, cell, range_ms[sat_index] + 0.00001 * sig);
            gps_msm7::set_phase_range_rate(sat, cell, rate_m_s[sat_index]);
            gps_msm7::set_cnr(cell, 45.0 - 5.0 * sig);
            cell.lock_time = 600U;
            static_cast<void>(epoch.signals.push_back(cell));
        }
    }

    return castle::succeeded(encode_message(epoch, frame, capacity, written));
}
```

See `samples/castle_ext/protocols/rtcm/messages/msm.cpp` for a realistic 3 satellite x 2 signal GPS MSM7 epoch built from physical ranges (header bytes, length, CRC-24Q), the validation errors, invalid markers and a GLONASS MSM4 with `set_glonass_epoch()`.

### Worked example

A GPS MSM4 message with station 1, epoch 1 ms, satellite ID 1, signal ID 2 and one cell (rough range 70.5 ms, fine pseudorange -2, fine phase-range 3, lock time 9, half-cycle set, CNR 45), `multiple_message = true`, `iods = 5`, `clock_steering = 2`, `external_clock = 1`, `divergence_free = true`, `smoothing_interval = 6`:

| Field | Bits | Value written |
| --- | --- | --- |
| DF002 | 12 | `010000110010` (1074) |
| DF003 | 12 | `000000000001` (1) |
| epoch | 30 | `000000000000000000000000000001` |
| DF393 | 1 | `1` |
| DF409 | 3 | `101` |
| DF001 | 7 | `0000000` |
| DF411 | 2 | `10` |
| DF412 | 2 | `01` |
| DF417 | 1 | `1` |
| DF418 | 3 | `110` |
| DF394 | 64 | `1` then 63 zeros (satellite ID 1) |
| DF395 | 32 | `01` then 30 zeros (signal ID 2) |
| DF396 | 1 | `1` (1 x 1 cells) |
| DF397 | 8 | `01000110` (70) |
| DF398 | 10 | `1000000000` (512 = 0.5 ms) |
| DF400 | 15 | `111111111111110` (-2) |
| DF401 | 22 | `0000000000000000000011` (3) |
| DF402 | 4 | `1001` (9) |
| DF420 | 1 | `1` |
| DF403 | 6 | `101101` (45) |
| padding | 4 | `0000` |

That is 236 bits, i.e. 30 payload bytes. The frame is 36 bytes:

```
d3 00 1e 43 20 01 00 00 00 07 40 4f 40 00 00 00 00 00 00 00 20 00 00 00 51 a0 0f ff c0 00 01 ce d0 58 ea 91
```

`d3` preamble, `00 1e` length 30, 30 payload bytes, `58 ea 91` CRC-24Q. The frame was decoded independently with the `pyrtcm` library (CRC verified); the unit test `HandComputedMsm4PayloadBits` asserts these exact bytes.

## Constraints & Notes

* **Encode-only.** There is no MSM decoder; the golden frames in the unit tests were verified with an external RTCM decoder, not with code in this library.
* **Capacity.** `satellites` holds at most `MaxSatellites` and `signals` at most `MaxCells` entries (each at most 64, `push_back()` returns `full` beyond). A mask that needs more entries than the capacity cannot match the vector size and is rejected with `invalid_argument`. Fixed-size, no heap allocation; the message is a plain value type (a full-capacity `msm_message<..., 64, 64>` is about 1.5 KiB).
* **Limits.** At most 64 satellites, 32 signals and 64 cells. `satellite_bit()`, `signal_bit()` and `cell_bit()` return `0` instead of a wrong bit for invalid input, so OR-ing them silently drops the entry; the mismatch is then reported by `encode()` as `invalid_argument`.
* **Quantisation.** Pseudorange and phase-range residuals are relative to the *quantised* rough range (1/1024 ms, about 293 m), so fine pseudoranges can differ from the rough range by at most 2^-10 ms (about 293 m) and fine phase-ranges by 2^-8 ms. Values outside become the invalid markers instead of being clamped; a value that rounds onto the marker (for example -2^14 steps) is also replaced by the marker so that valid data never collides with it.
* **Rounding.** Fine values and rates round half away from zero; rough range and CNR round half up on non-negative values. The rough range rounds to 1/1024 ms, so a value of 254.9995 ms or more may round up to 255 and then reads as invalid.
* **CNR.** `set_cnr()` maps NaN and negative input to 0; +/- infinity saturates.
* **Lock time.** `lock_time` is the raw RTCM lock-time indicator code, not a duration in seconds.
* **MSM4.** `extended_info`, `rough_phase_range_rate` and `fine_phase_range_rate` are not part of the MSM4 payload and are ignored (not range-checked). `set_phase_range_rate()` and the three-argument `set_rough_range()` do not compile for MSM4.
* **Error behavior.** No exceptions. Validation precedes any write, so an `invalid_argument` rejection writes nothing; a field overflow is reported as `out_of_range` rather than truncated (see the status table above).
* **Frame size.** The 1023-byte payload limit is never reached by a valid message (largest MSM7 is 958 bytes). `encode_message()` returns `full` when the output buffer is smaller than the frame.
* **Thread-safety:** none. A message object must not be shared between threads without external synchronisation; the `static` helpers are pure and safe to call concurrently on distinct objects.
