# HTTP Client Umbrella Header

## Overview
Includes the complete bounded HTTP client API: shared types, fixed-capacity headers, HPACK, HTTP/1.1 codecs, HTTP/2 framing/session, and the polled TCP client adapters.

## Header
`#include "castle_ext/comm/http/http.hpp"`

## Dependencies
- [HTTP shared types](types.md)
- [HTTP bounded headers](headers.md)
- [HPACK codec](hpack.md)
- [HTTP/1.1 codec](http1.md)
- [HTTP/2 session](http2.md)
- [HTTP client adapters](client.md)

## Public API
The umbrella header re-exports the declarations from all components listed above. Prefer including an individual component header when only one layer is required; include this header for application code that uses the complete client stack.

## Usage Example
```cpp
#include "castle_ext/comm/http/http.hpp"

int main()
{
    castle::comm::http::basic_headers<8U, 32U, 96U> headers;
    return headers.add("accept", "*/*") == castle::status::ok ? 0 : 1;
}
```

## Constraints & Notes
The umbrella itself defines no additional storage. All allocation, execution, and protocol constraints are those of the included component and selected template parameters. Including it makes HPACK and both protocol implementations visible to the translation unit.
