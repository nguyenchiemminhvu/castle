# CAN Sender and Receiver Nodes

## Overview

Provides small node facades that route validated CAN frames through inline `castle::callbacks::function` wrappers. Hardware drivers can connect to the sender callback and deliver received frames to the receiver without virtual dispatch, function-pointer context, or dynamic allocation.

## Header

`#include "castle_ext/protocols/can/node.hpp"`

## Dependencies

- [`castle/callbacks/function.hpp`](../../../../include/castle/callbacks/function.hpp)
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/config.hpp`](../../../../include/castle/core/config.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)
- [`castle_ext/protocols/can/frame.hpp`](frame.md)

## Public API

| API | Description |
|---|---|
| `sender_node<CallbackStorageSize, CallbackStorageAlignment>` | Sender node. Defaults to `castle::inplace_storage_reserved` and `castle::inplace_alignment_default`. |
| `receiver_node<CallbackStorageSize, CallbackStorageAlignment>` | Receiver node with the same storage parameters. |
| `sender_node::transmit_callback_type` | `castle::callbacks::function<castle::status(const frame&)>` adapter callback. |
| `receiver_node::receive_callback_type` | `castle::callbacks::function<void(const frame&)>` application callback. |
| `sender_node::configure(callback)` | Sets/replaces the hardware transmitter callback. Accepts a function pointer, lambda, or wrapper. |
| `sender_node::clear()` | Removes the transmitter callback. |
| `sender_node::send(frame)` | Validates then transmits a data or remote frame; propagates adapter status. |
| `sender_node::sent_count()` | Number of successful sends, modulo 2^32. |
| `receiver_node::configure(callback)` | Sets/replaces the incoming-frame callback. |
| `receiver_node::clear()` | Removes the incoming-frame callback. |
| `receiver_node::receive(frame)` | Validates and dispatches the frame, returning a deterministic status. |
| `receiver_node::received_count()` / `invalid_count()` | Valid and invalid frame-value counters, modulo 2^32. |

## Usage Example

```cpp
#include "castle_ext/protocols/can/node.hpp"
#include <assert.h>

int main()
{
    using namespace castle::protocols::can;
    uint32_t sends = 0U;
    sender_node<> sender;
    sender.configure([&sends](const frame&) {
        ++sends;
        return castle::status::ok;
    });

    const uint8_t byte = 0x55U;
    frame value{};
    assert(castle::succeeded(make_data_frame(0x120U, identifier_format::standard,
        frame_format::classical, &byte, 1U, value)));
    assert(castle::succeeded(sender.send(value)));
    assert(sends == 1U && sender.sent_count() == 1U);
    return 0;
}
```

See [`samples/castle_ext/protocols/can/node.cpp`](../../../../samples/castle_ext/protocols/can/node.cpp).

## Constraints & Notes

- No heap use, exceptions, RTTI, or virtual functions. Each node stores one inline callback wrapper whose size and alignment are set by the template parameters; captured state must fit in `CallbackStorageSize`.
- The node does not own a controller or queue. The integration layer defines ISR behavior, buffering, arbitration retries, bus-off recovery, and synchronization.
- `sender_node` rejects application requests for error/overload frames because those are controller-generated bus signaling. `receiver_node` can deliver those event values if the platform adapter reports them.
- Counters are not atomic or thread-safe; protect them externally if callbacks can run concurrently.
