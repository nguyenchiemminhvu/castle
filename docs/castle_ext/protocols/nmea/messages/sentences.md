# NMEA Sentence Umbrella

## Overview

Umbrella header that includes every decoded message/sentence header shipped in this extension set.

## Header

`#include "castle_ext/protocols/nmea/messages/messages.hpp"`

## Dependencies

- [`castle_ext/protocols/nmea/messages/dtm.hpp`](dtm.md)
- [`castle_ext/protocols/nmea/messages/gbs.hpp`](gbs.md)
- [`castle_ext/protocols/nmea/messages/gga.hpp`](gga.md)
- [`castle_ext/protocols/nmea/messages/gll.hpp`](gll.md)
- [`castle_ext/protocols/nmea/messages/gns.hpp`](gns.md)
- [`castle_ext/protocols/nmea/messages/gsa.hpp`](gsa.md)
- [`castle_ext/protocols/nmea/messages/gst.hpp`](gst.md)
- [`castle_ext/protocols/nmea/messages/gsv.hpp`](gsv.md)
- [`castle_ext/protocols/nmea/messages/rmc.hpp`](rmc.md)
- [`castle_ext/protocols/nmea/messages/vtg.hpp`](vtg.md)
- [`castle_ext/protocols/nmea/messages/zda.hpp`](zda.md)

## Public API

| API | Description |
| --- | --- |
| `dtm` | Included protocol model header. |
| `gbs` | Included protocol model header. |
| `gga` | Included protocol model header. |
| `gll` | Included protocol model header. |
| `gns` | Included protocol model header. |
| `gsa` | Included protocol model header. |
| `gst` | Included protocol model header. |
| `gsv` | Included protocol model header. |
| `rmc` | Included protocol model header. |
| `vtg` | Included protocol model header. |
| `zda` | Included protocol model header. |

## Usage Example

```cpp
#include "castle_ext/protocols/nmea/messages/messages.hpp"

using castle::protocols::nmea::message_view;
using castle::protocols::nmea::messages::gga;

void handle(const message_view& raw) {
    gga value{};
    (void)gga::decode(raw, value);
}
```

## Constraints & Notes

- This is an include-only umbrella; it does not declare a standalone sentence type.
- Including it makes all sentence models in the bundled NMEA sentence directory available to the translation unit.
