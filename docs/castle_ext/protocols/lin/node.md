# LIN Nodes

## Overview

Provides hardware-independent master and slave node facades. The master emits headers and master-owned responses and validates incoming responses. A slave prepares responses for requested identifiers and validates responses it subscribes to. Adapter and application callbacks are stored inline without heap allocation, exceptions, RTTI, or virtual dispatch.

## Header

`#include "castle_ext/protocols/lin/node.hpp"`

## Dependencies

- [`castle/callbacks/function.hpp`](../../../../include/castle/callbacks/function.hpp)
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/config.hpp`](../../../../include/castle/core/config.hpp)
- [`castle/core/traits.hpp`](../../../../include/castle/core/traits.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`castle/utility/forward.hpp`](../../../../include/castle/utility/forward.hpp)
- [`castle/utility/move.hpp`](../../../../include/castle/utility/move.hpp)
- [`frame.hpp`](frame.md)
- [`checksum.hpp`](checksum.md)
- [`schedule.hpp`](schedule.md)

## Public API

| API | Description |
|---|---|
| `master_node<StorageSize, Alignment>` | Configures header transmitter, response provider/transmitter, and receive handler callbacks. |
| `master_node::send_header(id)` | Emits a validated logical header through the configured adapter. |
| `master_node::send_master_response(id)` | Requests a frame from the provider, validates it, and transmits it. |
| `master_node::transmit_slot(entry)` | Emits a slot header, then publishes a response only for master-owned slots. |
| `master_node::receive_response(frame)` | Validates response structure/checksum and dispatches it. |
| `slave_node<StorageSize, Alignment>` | Configures a response provider and subscription handler. |
| `slave_node::handle_header(header, output)` | Provides a valid response for a published ID; provider may return `not_found` for other IDs. |
| `slave_node::handle_protected_identifier(pid, output)` | Validates/decodes PID and handles the associated header. |
| `slave_node::receive_response(frame)` | Validates and dispatches a subscribed response. |
| `clear_*()` and `*_count()` | Clear configured callbacks and read successful/invalid-operation counters. |

## Usage Example

```cpp
#include "castle_ext/protocols/lin/node.hpp"
#include <assert.h>

int main()
{
    using namespace castle::protocols::lin;
    uint8_t transmitted_id = 0xFFU;
    master_node<> master;
    master.configure_header_transmitter([&transmitted_id](const header& h) {
        transmitted_id = h.identifier;
        return castle::status::ok;
    });
    assert(castle::succeeded(master.send_header(0x12U)));
    assert(transmitted_id == 0x12U);
    return 0;
}
```

See [`samples/castle_ext/protocols/lin/node.cpp`](../../../../samples/castle_ext/protocols/lin/node.cpp).

## Constraints & Notes

Callback size and alignment are compile-time parameters and must fit stored callables. Callbacks must obey the library's no-exceptions integration requirements. Node counters wrap modulo 2^32. The application/driver owns physical-layer timing, schedule pacing, arbitration-free bus ownership, receive buffering, and any ISR/thread synchronization.
