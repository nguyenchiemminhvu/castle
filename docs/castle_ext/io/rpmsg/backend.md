# RPMsg Backend Contract

## Overview
Defines a structural compile-time contract for OpenAMP-compatible RPMsg transports without inheritance or virtual dispatch.

## Header
`#include "castle_ext/io/rpmsg/backend.hpp"`

## Dependencies
[`types.hpp`](../types.md), [`message.hpp`](../message.md), [`name_service.hpp`](../name_service.md), CASTLE compiler/type/status primitives.

## Public API
Callback dispatch types and `detail::has_backend_contract<Backend>`.

## Usage Example
```cpp
#include "castle_ext/io/rpmsg/backends/mock.hpp"
#include "castle_ext/io/rpmsg/backend.hpp"

static_assert(castle::io::rpmsg::detail::has_backend_contract<castle::io::rpmsg::mock::backend>::value, "mock backend contract");
int main() { return 0; }
```

## Constraints & Notes
A backend must provide initialize/shutdown/state, endpoint create/destroy, send, TX buffer acquire/commit/release, remote-address query, and poll methods with the signatures detected in this header. Dispatch uses function pointers plus `void*` contexts. Calls are synchronous; interrupt/thread safety and progress notification are backend/platform responsibilities. No virtual functions, heap allocation, or type erasure are introduced by the contract.
