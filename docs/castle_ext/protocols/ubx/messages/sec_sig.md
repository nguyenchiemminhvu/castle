# UBX SEC-SIG

## Overview

`sec_sig` models UBX-SEC-SIG (0x27 0x09), the signal security output reporting whether jamming / interference and spoofing detection are enabled and their current state. UBX-SEC-SIG is **not part of UBX-19056845** (ZED-F9R interface description); this model follows the generic u-blox version-1 layout of its 12-byte prefix. It is a [`fixed_message`](detail/fixed_message.md) with a custom `decode()` that tolerates the data later versions append (version 2 adds jamming-frequency blocks).

## Header

`#include "castle_ext/protocols/ubx/messages/sec_sig.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../core/compiler.md)
- [`castle_ext/protocols/ubx/ubx.hpp`](../ubx.md)
- [`detail/fixed_message.hpp`](detail/fixed_message.md)
- `stdint.h`

## Public API

All names live in `castle::protocols::ubx::messages`.

### Types

| Type | Description |
| --- | --- |
| `sec_sig_jamming_state : uint8_t` | `enum class` for `jammingState` (`jamFlags` bits 2..1): `unknown = 0` (unknown or detection disabled), `no_jamming = 1`, `warning = 2` (interference visible, fix OK), `critical = 3` (interference visible, no fix). |
| `sec_sig_spoofing_state : uint8_t` | `enum class` for `spoofingState` (`spfFlags` bits 3..1): `unknown = 0` (unknown or detection disabled), `no_spoofing = 1`, `indicated = 2` (spoofing indicated), `affirmed = 3` (spoofing affirmed). |
| `sec_sig` | The message; `fixed_message<sec_sig, UBX_CLASS_SEC, UBX_ID_SEC_SIG, 12U>`. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `sec_sig::msg_class` / `msg_id` | `UBX_CLASS_SEC` (`0x27`) / `UBX_ID_SEC_SIG` (`0x09`) | Class/ID, inherited from `fixed_message`. |
| `sec_sig::payload_length` | `12U` | Length of the decoded prefix and of the encoded payload. |
| `sec_sig::JAM_FLAGS_JAM_DET_ENABLED` | `0x01U` (bit 0) | `jamDetEnabled`. |
| `sec_sig::JAM_FLAGS_JAMMING_STATE` | `0x06U` (bits 2..1) | `jammingState`. |
| `sec_sig::SPF_FLAGS_SPF_DET_ENABLED` | `0x01U` (bit 0) | `spfDetEnabled`. |
| `sec_sig::SPF_FLAGS_SPOOFING_STATE` | `0x0EU` (bits 3..1) | `spoofingState`. |

### Fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `version` | `uint8_t` | - | Message version. Offset 0, followed by 3 reserved bytes. |
| `jam_flags` | `uint8_t` | - | Jamming information, see `JAM_FLAGS_*`. Offset 4, followed by 3 reserved bytes. |
| `spf_flags` | `uint8_t` | - | Spoofing information, see `SPF_FLAGS_*`. Offset 8, followed by 3 reserved bytes. |

### Functions

| Function | Description |
| --- | --- |
| `static bool sec_sig::matches(const message_view& raw)` | Class/ID check (inherited); any length. |
| `static bool sec_sig::decode(const message_view& raw, sec_sig& out)` | Requires class/ID and at least 12 payload bytes; decodes the first 12 bytes and ignores the rest. On failure `out` is value-initialised and `false` is returned. |
| `void sec_sig::encode(payload_writer& writer) const` | Inherited; writes exactly 12 bytes with zero reserved bytes. Used by `encode_message()` (loop-back). |
| `bool sec_sig::jam_det_enabled() const` | `JAM_FLAGS_JAM_DET_ENABLED` set. |
| `sec_sig_jamming_state sec_sig::jamming_state() const` | `(jam_flags & JAM_FLAGS_JAMMING_STATE) >> 1` as the enum. |
| `bool sec_sig::spf_det_enabled() const` | `SPF_FLAGS_SPF_DET_ENABLED` set. |
| `sec_sig_spoofing_state sec_sig::spoofing_state() const` | `(spf_flags & SPF_FLAGS_SPOOFING_STATE) >> 1` as the enum. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/sec_sig.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    sec_sig sig{};
    if (!sec_sig::decode(raw, sig))
    {
        return;
    }
    if (sig.jam_det_enabled() && (sig.jamming_state() == sec_sig_jamming_state::critical))
    {
        // interference visible and no fix
    }
    if (sig.spf_det_enabled() && (sig.spoofing_state() == sec_sig_spoofing_state::affirmed))
    {
        // spoofing affirmed
    }
}
```

See `samples/castle_ext/protocols/ubx/messages/sec_sig.cpp` for encode, decode, every accessor and the trailing-byte tolerance.

## Constraints & Notes

- Not part of UBX-19056845 (ZED-F9R): the layout is the generic u-blox version-1 12-byte prefix; `version` is stored, not checked.
- Length validation: `decode()` accepts any payload of length >= 12, decodes the 12-byte prefix and ignores the tail (version 2 appends jamming-frequency blocks, which this model does not expose); shorter payloads (or a different class/ID) return `false` with `out` value-initialised.
- `encode()` always writes exactly 12 bytes. `encode_message()` returns `castle::status::full` when the output buffer is smaller than the 20-byte frame.
- `spoofing_state()` casts the 3-bit field directly; wire values 4..7 yield a `sec_sig_spoofing_state` value outside the named enumerators.
- API change: the former stored fields `jam_det_enabled`, `jamming_state`, `spf_det_enabled` and `spoofing_state` are now accessor functions over `jam_flags` / `spf_flags`, returning `bool` / `sec_sig_jamming_state` / `bool` / `sec_sig_spoofing_state`. A decoded value therefore cannot hold contradictory copies of the same bits. Migrate `sig.jamming_state` to `sig.jamming_state()` (and likewise for the other three); to build a value, set the bits in `jam_flags` / `spf_flags` with the `JAM_FLAGS_*` / `SPF_FLAGS_*` masks.
- No heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
