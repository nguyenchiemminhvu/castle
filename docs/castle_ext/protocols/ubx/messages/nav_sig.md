# UBX NAV-SIG

## Overview

`nav_sig<MaxSignals>` models UBX-NAV-SIG (0x01 0x43), the periodic/polled signal information output: an 8-byte header followed by `numSigs` 16-byte signal blocks (`8 + numSigs·16`). Each block is a `nav_sig_info` with typed quality / correction-source / iono-model fields and accessors for every `sigFlags` sub-field.

## Header

`#include "castle_ext/protocols/ubx/messages/nav_sig.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle/core/constants.hpp`](../../../../core/constants.md)
- [`castle/core/types.hpp`](../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../container/vector.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../payload_writer.md)
- [`detail/repeated_group.hpp`](detail/repeated_group.md)
- [`castle_ext/protocols/ubx/messages/nav_sat.hpp`](nav_sat.md) (`nav_sv_health`, `nav_signal_quality`)

## Public API

### Types

| Type | Description |
| --- | --- |
| `nav_sig_corr_source : uint8_t` | `none = 0`, `sbas = 1`, `beidou = 2`, `rtcm2 = 3`, `rtcm3_osr = 4`, `rtcm3_ssr = 5`, `qzss_slas = 6`, `spartn = 7`. |
| `nav_sig_iono_model : uint8_t` | `none = 0`, `klobuchar_gps = 1`, `sbas = 2`, `klobuchar_beidou = 3`, `dual_frequency = 8`. |
| `nav_sig_info` | One 16-byte signal block. |
| `nav_sig<MaxSignals = 64U>` | The message; `MaxSignals <= 255`. |

### Constants

| Constant | Value |
| --- | --- |
| `nav_sig::msg_class` / `msg_id` | `UBX_CLASS_NAV` / `UBX_ID_NAV_SIG` |
| `nav_sig::header_length` | `8U` |
| `nav_sig::block_length` / `nav_sig_info::wire_size` | `16U` |
| `nav_sig_info::SIG_FLAGS_HEALTH` | `0x0003` (bits 1..0) |
| `nav_sig_info::SIG_FLAGS_PR_SMOOTHED` | `0x0004` (bit 2) |
| `nav_sig_info::SIG_FLAGS_PR_USED` / `CR_USED` / `DO_USED` | bits 3 / 4 / 5 |
| `nav_sig_info::SIG_FLAGS_PR_CORR_USED` / `CR_CORR_USED` / `DO_CORR_USED` | bits 6 / 7 / 8 |

### `nav_sig_info` fields

| Field | Type | Unit | Description |
| --- | --- | --- | --- |
| `gnss_id` | `uint8_t` | - | GNSS identifier. |
| `sv_id` | `uint8_t` | - | Satellite identifier. |
| `sig_id` | `uint8_t` | - | New style signal identifier. |
| `freq_id` | `uint8_t` | - | GLONASS frequency slot + 7 (0..13). |
| `pr_res` | `int16_t` | 0.1 m | Pseudorange residual. |
| `cno` | `uint8_t` | dBHz | Carrier-to-noise density ratio. |
| `qual_ind` | `nav_signal_quality` | - | Spec `qualityInd`. |
| `corr_source` | `nav_sig_corr_source` | - | Correction source. |
| `iono_model` | `nav_sig_iono_model` | - | Ionospheric model used. |
| `sig_flags` | `uint16_t` | - | Bitmask, see `SIG_FLAGS_*`. |

### `nav_sig` fields

| Field | Type | Unit | Description |
| --- | --- | --- | --- |
| `i_tow` | `uint32_t` | ms | GPS time of week of the navigation epoch. |
| `version` | `uint8_t` | - | Message version (0x00). |
| `num_sigs` | `uint8_t` | - | On-wire signal count; may exceed `signals.size()`. |
| `signals` | `castle::container::vector<nav_sig_info, MaxSignals>` | - | Stored signal blocks. |

### Functions

| Function | Description |
| --- | --- |
| `nav_sig_info::health()` | `nav_sv_health` from bits 1..0. |
| `nav_sig_info::pr_smoothed()`, `pr_used()`, `cr_used()`, `do_used()`, `pr_corr_used()`, `cr_corr_used()`, `do_corr_used()` | Single-bit flags. |
| `nav_sig_info::pr_res_m()` | Pseudorange residual in metres. |
| `static bool nav_sig::matches(const message_view&)` | Class/ID check. |
| `static bool nav_sig::decode(const message_view&, nav_sig&)` | Validates the exact length, decodes the header and up to `MaxSignals` blocks. |
| `void nav_sig::encode(payload_writer&) const` | Writes the header (count from `signals.size()`) and every stored block; use with `encode_message()`. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/nav_sig.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    nav_sig<64U> sig;
    if (!nav_sig<64U>::decode(raw, sig))
    {
        return;
    }
    for (const nav_sig_info& info : sig.signals)
    {
        if (info.pr_used() && (info.corr_source != nav_sig_corr_source::none))
        {
            // corrected pseudorange in use ...
        }
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/nav_sig.cpp` for encode, decode and every accessor.

## Constraints & Notes

- Capacity is `MaxSignals` (default 64). Blocks beyond capacity are skipped; `num_sigs` keeps the on-wire count while `signals.size()` is the stored count.
- No heap allocation; the vector is embedded in the value.
- `decode()` returns `false` and resets the output unless the class/ID match and the payload length is exactly `8 + numSigs·16`.
- `encode()` takes `numSigs` from `signals.size()`; reserved bytes are written as zero.
- Enumerated fields store the raw wire byte; values outside the listed enumerators (reserved codes) are preserved, not rejected.
- Thread-safety: none; plain value type, not thread-safe.
