# UBX Message Umbrella

## Overview

Umbrella header that includes every decoded message/sentence header shipped in this extension set.

## Header

`#include "castle_ext/protocols/ubx/messages/messages.hpp"`

## Dependencies

- [`castle_ext/protocols/ubx/messages/ack.hpp`](ack.md)
- [`castle_ext/protocols/ubx/messages/cfg_valget.hpp`](cfg_valget.md)
- [`castle_ext/protocols/ubx/messages/esf_ins.hpp`](esf_ins.md)
- [`castle_ext/protocols/ubx/messages/esf_meas.hpp`](esf_meas.md)
- [`castle_ext/protocols/ubx/messages/esf_status.hpp`](esf_status.md)
- [`castle_ext/protocols/ubx/messages/inf.hpp`](inf.md)
- [`castle_ext/protocols/ubx/messages/mon_io.hpp`](mon_io.md)
- [`castle_ext/protocols/ubx/messages/mon_span.hpp`](mon_span.md)
- [`castle_ext/protocols/ubx/messages/mon_txbuf.hpp`](mon_txbuf.md)
- [`castle_ext/protocols/ubx/messages/mon_ver.hpp`](mon_ver.md)
- [`castle_ext/protocols/ubx/messages/nav2_dop.hpp`](nav2_dop.md)
- [`castle_ext/protocols/ubx/messages/nav2_eell.hpp`](nav2_eell.md)
- [`castle_ext/protocols/ubx/messages/nav2_pvt.hpp`](nav2_pvt.md)
- [`castle_ext/protocols/ubx/messages/nav2_timegps.hpp`](nav2_timegps.md)
- [`castle_ext/protocols/ubx/messages/nav_att.hpp`](nav_att.md)
- [`castle_ext/protocols/ubx/messages/nav_clock.hpp`](nav_clock.md)
- [`castle_ext/protocols/ubx/messages/nav_dop.hpp`](nav_dop.md)
- [`castle_ext/protocols/ubx/messages/nav_eell.hpp`](nav_eell.md)
- [`castle_ext/protocols/ubx/messages/nav_odo.hpp`](nav_odo.md)
- [`castle_ext/protocols/ubx/messages/nav_pvt.hpp`](nav_pvt.md)
- [`castle_ext/protocols/ubx/messages/nav_sat.hpp`](nav_sat.md)
- [`castle_ext/protocols/ubx/messages/nav_sig.hpp`](nav_sig.md)
- [`castle_ext/protocols/ubx/messages/nav_status.hpp`](nav_status.md)
- [`castle_ext/protocols/ubx/messages/nav_timegps.hpp`](nav_timegps.md)
- [`castle_ext/protocols/ubx/messages/nav_timeutc.hpp`](nav_timeutc.md)
- [`castle_ext/protocols/ubx/messages/rxm_measx.hpp`](rxm_measx.md)
- [`castle_ext/protocols/ubx/messages/sec_crc.hpp`](sec_crc.md)
- [`castle_ext/protocols/ubx/messages/sec_sig.hpp`](sec_sig.md)
- [`castle_ext/protocols/ubx/messages/tim_tp.hpp`](tim_tp.md)
- [`castle_ext/protocols/ubx/messages/upd_sos.hpp`](upd_sos.md)

## Public API

| API | Description |
| --- | --- |
| `ack` | Included protocol model header. |
| `cfg_valget` | Included protocol model header. |
| `esf_ins` | Included protocol model header. |
| `esf_meas` | Included protocol model header. |
| `esf_status` | Included protocol model header. |
| `inf` | Included protocol model header. |
| `mon_io` | Included protocol model header. |
| `mon_span` | Included protocol model header. |
| `mon_txbuf` | Included protocol model header. |
| `mon_ver` | Included protocol model header. |
| `nav2_dop` | Included protocol model header. |
| `nav2_eell` | Included protocol model header. |
| `nav2_pvt` | Included protocol model header. |
| `nav2_timegps` | Included protocol model header. |
| `nav_att` | Included protocol model header. |
| `nav_clock` | Included protocol model header. |
| `nav_dop` | Included protocol model header. |
| `nav_eell` | Included protocol model header. |
| `nav_odo` | Included protocol model header. |
| `nav_pvt` | Included protocol model header. |
| `nav_sat` | Included protocol model header. |
| `nav_sig` | Included protocol model header. |
| `nav_status` | Included protocol model header. |
| `nav_timegps` | Included protocol model header. |
| `nav_timeutc` | Included protocol model header. |
| `rxm_measx` | Included protocol model header. |
| `sec_crc` | Included protocol model header. |
| `sec_sig` | Included protocol model header. |
| `tim_tp` | Included protocol model header. |
| `upd_sos` | Included protocol model header. |

## Usage Example

```cpp
#include "castle_ext/protocols/ubx/messages/messages.hpp"

using castle::protocols::ubx::message_view;
using castle::protocols::ubx::messages::nav_pvt;

void handle(const message_view& raw) {
    nav_pvt value{};
    (void)nav_pvt::decode(raw, value);
}
```

## Constraints & Notes

- This is an include-only umbrella; it does not declare a standalone decoded message type.
- Including it makes all message models in the bundled UBX message directory available to the translation unit.
