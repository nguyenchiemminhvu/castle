# RTCM GLONASS RTK observables (1009 - 1012)

## Overview

`glonass_rtk_observables<Type, MaxSatellites>` builds the legacy RTCM 3 GLONASS RTK observation messages 1009 to 1012. They carry the base station's L1 (and optionally L2) pseudorange, carrier phase, lock time and signal-to-noise per satellite, plus the frequency channel of each GLONASS slot, which a rover such as a u-blox ZED-F9R uses as correction input.

| Type | Alias | Content | Bits per satellite |
| --- | --- | --- | --- |
| 1009 | `glonass_rtk_l1_obs` | L1-only | 64 |
| 1010 | `glonass_rtk_l1_obs_ext` | L1-only, extended (ambiguity, CNR) | 79 |
| 1011 | `glonass_rtk_l1_l2_obs` | L1 and L2 | 107 |
| 1012 | `glonass_rtk_l1_l2_obs_ext` | L1 and L2, extended | 130 |

As for the MSM models, there is **one type per message number**: the template argument `Type` fixes the message number at compile time, and the aliases above select it with the full capacity of 31 satellites. `Type` decides which satellite fields are written; fields a message does not carry are **ignored** (neither written nor range-checked). Satellite values are the raw integers sent on the wire, not scaled physical values; the `*_RESOLUTION_M` constants give their scale.

This is an **encode-only** model: it produces the message payload for `encode_message()` and there is no decoder.

## Header

`#include "castle_ext/protocols/rtcm/messages/glonass_rtk_observables.hpp"`

## Dependencies

- [`core/compiler.hpp`](../../../../core/compiler.md)
- [`core/types.hpp`](../../../../core/types.md)
- [`container/vector.hpp`](../../../../container/vector.md)
- [`error/status.hpp`](../../../../error/status.md)
- [`castle_ext/protocols/rtcm/bit_writer.hpp`](../bit_writer.md)
- [`castle_ext/protocols/rtcm/rtcm.hpp`](../rtcm.md)
- [`castle_ext/protocols/rtcm/messages/detail/rtk_observables_family.hpp`](detail/rtk_observables_family.md)

## Public API

All names live in `castle::protocols::rtcm::messages`.

### Types

| Type | Description |
| --- | --- |
| `glonass_rtk_observables<uint16_t Type, castle::size_type MaxSatellites = 31U>` | The message, derived from `detail::rtk_observables_family<RTCM_TYPE_GLO_L1_OBS, Type>`. |
| `glonass_rtk_l1_obs`, `glonass_rtk_l1_obs_ext`, `glonass_rtk_l1_l2_obs`, `glonass_rtk_l1_l2_obs_ext` | Aliases of `glonass_rtk_observables<1009 .. 1012>` with full capacity (see the overview table). |
| `glonass_rtk_satellite` | Per-satellite wire values (below). |

`Type` must be one of `RTCM_TYPE_GLO_L1_OBS` .. `RTCM_TYPE_GLO_L1L2_OBS_EXT` (1009..1012); any other number is rejected by a `static_assert` in [`detail::rtk_observables_family`](detail/rtk_observables_family.md), which the GPS family shares. `MaxSatellites` is the satellite capacity; a `static_assert` requires `1 <= MaxSatellites <= 31` because the satellite count (DF035) is a 5-bit field. A custom capacity names the number explicitly, for example `glonass_rtk_observables<RTCM_TYPE_GLO_L1L2_OBS_EXT, 8U>`. Both checks are compile errors and are not reported at run time.

The satellite vector type depends only on `MaxSatellites`, so models of different numbers with the same capacity can share their satellites (`l1_l2.satellites = l1.satellites;`).

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `MESSAGE_TYPE` | `uint16_t`, `Type` | DF002, written first. |
| `IS_EXTENDED` | `true` for 1010, 1012 | The message carries `l1_ambiguity`, `l1_cnr` (and `l2_cnr` with L2). |
| `HAS_L2` | `true` for 1011, 1012 | The message carries the L2 block. |
| `FIRST_TYPE` / `LAST_TYPE` | `1009` / `1012` | Number range of the family. |
| `PSEUDORANGE_RESOLUTION_M` | `0.02` | Metres per unit of `l1_pseudorange` and `l2_pseudorange_difference`. |
| `PHASE_RESOLUTION_M` | `0.0005` | Metres per unit of the phase-minus-pseudorange fields. |
| `AMBIGUITY_M` | `599584.916` | Metres per unit of `l1_ambiguity`. |
| `CNR_RESOLUTION_DBHZ` | `0.25` | dB-Hz per unit of `l1_cnr` and `l2_cnr`. |

`MESSAGE_TYPE`, `IS_EXTENDED`, `HAS_L2`, `FIRST_TYPE`, `LAST_TYPE` and `accepts()` are inherited from `detail::rtk_observables_family`.

### Message fields

| Field | Type | Wire format | Description |
| --- | --- | --- | --- |
| `station_id` | `uint16_t` | DF003, 12 bit | Reference station ID, 0..4095. |
| `time_of_day_ms` | `uint32_t` | DF034, 27 bit | GLONASS epoch time tk, 0..0x7FFFFFF ms. |
| `synchronous` | `bool` | DF005 | More messages follow for this epoch. |
| `divergence_free` | `bool` | DF036 | Divergence-free smoothing indicator. |
| `smoothing_interval` | `uint8_t` | DF037, 3 bit | Smoothing interval, 0..7. |
| `satellites` | `castle::container::vector<glonass_rtk_satellite, MaxSatellites>` | one block each | The satellite count DF035 (5 bit) is `satellites.size()`; 0 satellites is valid. |

### Satellite fields (`glonass_rtk_satellite`)

| Field | Type | Wire format | Carried by | Description |
| --- | --- | --- | --- | --- |
| `satellite_id` | `uint8_t` | DF038, 6 bit | all | GLONASS slot number, 0..63 (1..24 in practice; 40..63 reserved for non-GLONASS). |
| `l1_code` | `uint8_t` | DF039, 1 bit | all | 0 = C/A, 1 = P. |
| `frequency_channel` | `uint8_t` | DF040, 5 bit | all | Frequency channel number + 7 (0..20 in practice), 0..31 accepted. |
| `l1_pseudorange` | `uint32_t` | DF041, 25 bit | all | L1 pseudorange modulo ambiguity, 0.02 m, 0..0x1FFFFFF. |
| `l1_phase_minus_pseudorange` | `int32_t` | DF042, 20 bit signed | all | L1 phaserange - pseudorange, 0.0005 m, -524288..524287. |
| `l1_lock_time` | `uint8_t` | DF043, 7 bit | all | L1 lock time indicator, 0..127. |
| `l1_ambiguity` | `uint8_t` | DF044, 7 bit | 1010, 1012 | Integer L1 pseudorange ambiguity, 599584.916 m, 0..127. |
| `l1_cnr` | `uint8_t` | DF045, 8 bit | 1010, 1012 | L1 CNR, 0.25 dB-Hz. |
| `l2_code` | `uint8_t` | DF046, 2 bit | 1011, 1012 | 0 = C/A, 1 = P. |
| `l2_pseudorange_difference` | `int16_t` | DF047, 14 bit signed | 1011, 1012 | L2 - L1 pseudorange, 0.02 m, -8192..8191. |
| `l2_phase_minus_l1_pseudorange` | `int32_t` | DF048, 20 bit signed | 1011, 1012 | L2 phaserange - L1 pseudorange, 0.0005 m, -524288..524287. |
| `l2_lock_time` | `uint8_t` | DF049, 7 bit | 1011, 1012 | L2 lock time indicator, 0..127. |
| `l2_cnr` | `uint8_t` | DF050, 8 bit | 1012 | L2 CNR, 0.25 dB-Hz. |

### Functions

| Function | Returns | Description | Complexity |
| --- | --- | --- | --- |
| `static constexpr bool accepts(uint16_t type)` | `bool` | `type == MESSAGE_TYPE`, so `glonass_rtk_l1_obs::accepts(1010U)` is `false`. | O(1) |
| `void encode(bit_writer& writer) const` | - | Writes `MESSAGE_TYPE`, the remaining 49 header bits and the per-satellite bits selected by `IS_EXTENDED` / `HAS_L2` (61 header bits in total). Used by `encode_message()`. | O(satellites) |

### Encode results

`encode_message(observables, output, capacity, written)` returns:

| Status | Cause |
| --- | --- |
| `ok` | Frame written; `6 + ceil((61 + n * bits_per_satellite) / 8)` bytes for `n` satellites, at most 518 bytes (31 satellites of 1012). |
| `invalid_argument` | `output` is `nullptr`. |
| `out_of_range` | A carried field does not fit its width (for example `station_id` > 4095, `time_of_day_ms` > 0x7FFFFFF, `smoothing_interval` > 7, `frequency_channel` > 31, `l1_pseudorange` > 0x1FFFFFF, `l1_ambiguity` > 127, a 20-bit phase outside -524288..524287, `l2_pseudorange_difference` outside -8192..8191, `l1_lock_time` > 127, `l2_code` > 3). The value check runs before the space check, so an oversized value reports `out_of_range` even in a buffer that is too small. |
| `full` | `capacity` is smaller than the frame (including any `capacity` below 6). |

`written` is `0` on every failure. Only the first failure is reported. A wrong message number cannot reach `encode()`: it does not compile.

## Usage Example

```cpp
#include "castle_ext/protocols/rtcm/messages/glonass_rtk_observables.hpp"

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

castle::status encode_epoch(uint8_t* frame, castle::size_type capacity, castle::size_type& written)
{
    using observables_type = glonass_rtk_observables<RTCM_TYPE_GLO_L1L2_OBS_EXT, 12U>; // 1012, 12 satellites

    observables_type observables{};
    observables.station_id = 77U;
    observables.time_of_day_ms = 12345678U;
    observables.smoothing_interval = 5U;

    // 21 000 000.46 m = 35 ambiguities + a remainder in 0.02 m units.
    const double range_m = 21000000.46;
    const uint8_t ambiguity = static_cast<uint8_t>(range_m / observables_type::AMBIGUITY_M);
    const double remainder_m = range_m - (static_cast<double>(ambiguity) * observables_type::AMBIGUITY_M);

    glonass_rtk_satellite satellite{};
    satellite.satellite_id = 5U;
    satellite.frequency_channel = 7U + 1U; // channel +1
    satellite.l1_ambiguity = ambiguity;
    satellite.l1_pseudorange = static_cast<uint32_t>((remainder_m / observables_type::PSEUDORANGE_RESOLUTION_M) + 0.5);
    satellite.l1_phase_minus_pseudorange = -12345;
    satellite.l1_lock_time = 100U;
    satellite.l1_cnr = 160U; // 40 dB-Hz
    satellite.l2_code = 1U;
    satellite.l2_pseudorange_difference = -5000;
    satellite.l2_phase_minus_l1_pseudorange = 54321;
    satellite.l2_lock_time = 90U;
    satellite.l2_cnr = 150U;

    castle::status result = observables.satellites.push_back(satellite);
    if (!castle::succeeded(result))
    {
        return result; // status::full after 12 satellites
    }
    return encode_message(observables, frame, capacity, written);
}
```

To send the same epoch as 1009, fill a `glonass_rtk_observables<RTCM_TYPE_GLO_L1_OBS, 12U>` with the same header fields and copy `satellites` over; it writes only the L1 fields.

See [`samples/castle_ext/protocols/rtcm/messages/glonass_rtk_observables.cpp`](../../../../../samples/castle_ext/protocols/rtcm/messages/glonass_rtk_observables.cpp).

## Constraints & Notes

- Encode only: there is no parser or decoder for 1009..1012.
- One type per message number. Emitting several numbers for one epoch needs one object per number; there is no run-time message-number field to change.
- Satellite capacity is at most 31; adding more than `MaxSatellites` satellites is rejected by `vector::push_back()` with `status::full`, not by `encode()`. The `static_assert`s on `Type` and `MaxSatellites` fire at compile time only.
- Fields a message does not carry are ignored, including out-of-range raw values: a 1009 ignores `l1_ambiguity`, `l1_cnr` and the whole L2 block, a 1010 ignores the L2 block, and a 1011 ignores `l1_ambiguity`, `l1_cnr` and `l2_cnr`.
- Values are raw wire integers. The model does not split a range into ambiguity and remainder, does not compute lock-time indicators and does not validate satellite IDs against 1..24 or channels against 0..20.
- Unused bits of the last payload byte are written as zero (RTCM padding).
- No heap allocation, exceptions, RTTI or STL; the payload is written straight into the caller's frame buffer.
- Thread-safety: none. Instances are plain values; `encode()` does not modify the message.
