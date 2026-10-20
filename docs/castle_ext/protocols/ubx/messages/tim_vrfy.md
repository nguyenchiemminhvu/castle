# UBX TIM-VRFY

## Overview

`tim_vrfy` models UBX-TIM-VRFY (0x0D 0x06), the periodic/polled sourced time verification output. It compares the time previously received via assistance data or from the RTC with the current receiver time: the sourced time of week and week number, the delta (current time minus sourced time), and the aiding time source. It is a [`fixed_message`](detail/fixed_message.md).

## Header

`#include "castle_ext/protocols/ubx/messages/tim_vrfy.hpp"`

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
| `tim_vrfy_src : uint8_t` | `enum class` for `src` (`flags` bits 2..0): `no_aiding = 0` (no time aiding done), `rtc = 2` (source was the RTC), `assistance = 3` (source was assistance data). |
| `tim_vrfy` | The message; `fixed_message<tim_vrfy, UBX_CLASS_TIM, UBX_ID_TIM_VRFY, 20U>`. |

### Constants

| Constant | Value | Description |
| --- | --- | --- |
| `tim_vrfy::msg_class` / `msg_id` | `UBX_CLASS_TIM` (`0x0D`) / `UBX_ID_TIM_VRFY` (`0x06`) | Class/ID, inherited from `fixed_message`. |
| `tim_vrfy::payload_length` | `20U` | Exact payload length. |
| `tim_vrfy::FLAGS_SRC` | `0x07U` (bits 2..0) | `src`. |

### Fields

| Field | Type | Unit / scale | Description |
| --- | --- | --- | --- |
| `itow` | `int32_t` | ms | Integer millisecond time of week received by the source. Offset 0. |
| `frac` | `int32_t` | ns | Sub-millisecond part of the time of week. Offset 4. |
| `delta_ms` | `int32_t` | ms | Integer milliseconds of current time minus sourced time. Offset 8. |
| `delta_ns` | `int32_t` | ns | Sub-millisecond part of the delta time. Offset 12. |
| `wno` | `uint16_t` | weeks | Week number. Offset 16. |
| `flags` | `uint8_t` | - | Flags, see `FLAGS_*`. Offset 18, followed by 1 reserved byte. |

### Functions

| Function | Description |
| --- | --- |
| `static bool tim_vrfy::matches(const message_view& raw)` | Class/ID check (inherited). |
| `static bool tim_vrfy::decode(const message_view& raw, tim_vrfy& out)` | Inherited: requires class/ID and exactly 20 payload bytes; on failure `out` is value-initialised and `false` is returned. |
| `void tim_vrfy::encode(payload_writer& writer) const` | Inherited; writes a zero reserved byte. Used by `encode_message()` (loop-back). |
| `tim_vrfy_src tim_vrfy::src() const` | `flags & FLAGS_SRC` as the enum. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/tim_vrfy.hpp"

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;

void handle(const message_view& raw)
{
    tim_vrfy verify{};
    if (!tim_vrfy::decode(raw, verify) || (verify.src() == tim_vrfy_src::no_aiding))
    {
        return;
    }
    // verify.delta_ms ms + verify.delta_ns ns: drift of the RTC / assistance time
}
```

See `samples/castle_ext/protocols/ubx/messages/tim_vrfy.cpp` for encode, decode and every source value.

## Constraints & Notes

- Length validation: `decode()` requires exactly 20 payload bytes; shorter or longer payloads (or a different class/ID) return `false` with `out` value-initialised.
- `src` value 1 is not defined by the spec, so `tim_vrfy_src` has no enumerator for it; `src()` casts the 3-bit field directly, and wire values 1 and 4..7 yield values outside the named enumerators.
- The sub-millisecond fields are integer nanoseconds; no scaled helpers are needed.
- `encode_message()` returns `castle::status::full` when the output buffer is smaller than the 28-byte frame.
- No heap allocation, no exceptions.
- `message_view` is non-owning; its payload must stay valid while `decode()` runs. The decoded model owns its data.
- Value types; copy freely.
- Thread-safety: none. Not thread-safe; concurrent access to one object must be synchronised externally.
