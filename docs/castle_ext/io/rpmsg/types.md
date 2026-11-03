# RPMsg Types and Configuration

## Overview
Platform-neutral RPMsg constants, compile-time capacity options, role selection, shared-memory configuration, and address/timeout helpers.

## Header
`#include "castle_ext/io/rpmsg/types.hpp"`

## Dependencies
None. The header uses only CASTLE compiler/type/status primitives and `<stdint.h>`.

## Public API
Types, roles, RPMsg protocol constants, `options<>`, `device_config`, and validation helpers.

## Usage Example
```cpp
#include "castle_ext/io/rpmsg/types.hpp"

using namespace castle::io::rpmsg;
using small_options = options<4U, 4U, 20U, 128U, 8U, 256U>;
static_assert(small_options::max_endpoints == 4U, "fixed endpoint table");
int main()
{
    device_config config(role::remote, nullptr, 0U, 8U, 256U);
    return validate_config(config) == castle::status::ok && is_valid_local_address(1024U) ? 0 : 1;
}
```

## Constraints & Notes
All capacities are compile-time constants; runtime device buffer count/size may be lower but cannot exceed `Options::buffer_count` and `Options::buffer_size`. `timeout_type` is signed milliseconds: `0` requests non-blocking behavior, `-1` requests the backend's blocking operation, and positive finite values are accepted only by backends that can honor them. OpenAMP's blocking send uses its own bounded wait (currently documented as 15 seconds), not an infinite wait. `RPMSG_ADDR_ANY` is `0xffffffff`; addresses below 1024 are reserved by the RPMsg/OpenAMP convention. The default 512-byte buffer leaves 496 bytes for payload after the 16-byte transport header.
