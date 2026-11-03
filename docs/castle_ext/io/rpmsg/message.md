# RPMsg Message Views and TX Buffers

## Overview
Non-owning receive-message views and explicit handles for zero-copy transmit buffers.

## Header
`#include "castle_ext/io/rpmsg/message.hpp"`

## Dependencies
[`types.hpp`](../types.md), `castle/core/compiler.hpp`, `castle/core/types.hpp`.

## Public API
`message_view`, `tx_buffer`.

## Usage Example
```cpp
#include "castle_ext/io/rpmsg/message.hpp"

int main()
{
    const uint8_t bytes[] = {1U, 2U};
    castle::io::rpmsg::message_view view(1024U, 2048U, bytes, sizeof(bytes));
    castle::io::rpmsg::tx_buffer tx;
    tx.data = nullptr;
    tx.active = true;
    tx.reset();
    return view.empty() || tx.active ? 1 : 0;
}
```

## Constraints & Notes
`message_view::data` is borrowed from the OpenAMP RX buffer and is valid only during the receive callback unless the platform explicitly holds the RX buffer through a separate OpenAMP mechanism. `tx_buffer` is a move-only non-owning handle; its active ownership cannot be copied. A moved-from handle is cleared. Ownership returns to OpenAMP after successful `send_nocopy()` or `release_tx_buffer()`.
