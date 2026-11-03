# OpenAMP RPMsg Backend

## Overview
Optional adapter from CASTLE's structural backend contract to OpenAMP's RPMsg-over-VirtIO implementation. It can be used by Linux userspace or Zephyr when those platforms provide compatible OpenAMP/libmetal resources.

## Header
`#include "castle_ext/io/rpmsg/backends/openamp.hpp"`

## Dependencies
[`backend.hpp`](../../backend.md), [`types.hpp`](../../types.md), OpenAMP `<openamp/rpmsg.h>`, `<openamp/rpmsg_virtio.h>`, `<openamp/virtio.h>`, and `<metal/io.h>`.

## Public API
`openamp_resources` and `openamp_backend<MaxEndpoints>` (available only when `CASTLE_RPMSG_ENABLE_OPENAMP` is defined).

## Usage Example
```cpp
// Compile this sample without CASTLE_RPMSG_ENABLE_OPENAMP to verify the optional
// adapter header is safely excluded when the OpenAMP SDK is not part of the build.
#include "castle_ext/io/rpmsg/backends/openamp.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"

int main()
{
    static_assert(castle::io::rpmsg::default_buffer_size == 512U, "RPMsg default buffer");
#if defined(CASTLE_RPMSG_ENABLE_OPENAMP)
    // The application supplies preconfigured VirtIO, RPMsg VirtIO, and metal IO objects.
    // See openamp_resources and the platform resource-table/mailbox setup.
#endif
    return 0;
}
```

## Constraints & Notes
Define `CASTLE_RPMSG_ENABLE_OPENAMP` and provide OpenAMP/libmetal include and link dependencies to enable the adapter. Before constructing it, platform glue must configure the VirtIO device role, resource table/vrings, shared-memory IO region and cache attributes, and mailbox/IPI notifications. `openamp_backend` initializes the OpenAMP shared-memory pool and RPMsg VirtIO device, creates/destroys native endpoints, and uses OpenAMP send/no-copy calls. Both the device-level Name Service bind/unbind callbacks and endpoint-level unbind callback are connected to the common facade. The fixed global NS callback router supports up to four concurrent backend instances; initialization/shutdown of these instances must be serialized. Multiple local endpoints may use one service name, but OpenAMP binds each NS address to a matching endpoint rather than broadcasting one announcement to every same-name endpoint; configure peer destinations explicitly or create the endpoint from the corresponding NS callback. OpenAMP's blocking send has a backend-defined bounded wait; positive finite millisecond timeouts are rejected because this adapter cannot enforce arbitrary deadlines. The pool consumes the caller-supplied shared-memory region for `role::host` only; a `role::remote` backend may pass no region because the host owns the buffers. Actual buffer count/layout is defined by OpenAMP/VirtIO configuration, not by CASTLE's compile-time metadata alone. The RX trampoline always returns success to OpenAMP, which asserts on negative callback status; frames rejected by the device are dropped. `send()` returns `data_loss` when OpenAMP truncated the payload to a smaller peer buffer and `time_out` when a `wait_forever` send found no buffer. Name Service discovery requires the VirtIO RPMsg NS feature to be enabled by the peer/resource configuration. Zephyr's OpenAMP, resource-table, RPMsg-service and static-vring integration must be prepared by the Zephyr application. Linux kernel RPMsg char-device access is not implemented by this userspace adapter. Platform integration must drive virtqueue/mailbox processing. RX callbacks execute in the context chosen by that platform and must obey its locking/ISR constraints. This adapter was not compiled in the host build unless an OpenAMP SDK is installed.
