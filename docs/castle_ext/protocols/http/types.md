# HTTP Types

## Overview
Defines the shared HTTP method, version, error, header-view, and response-view types used by the bounded HTTP client. The views are non-owning; protocol clients own the memory they reference.

## Header
`#include "castle_ext/protocols/http/types.hpp"`

## Dependencies
- [CASTLE compiler annotations](../../../../include/castle/core/compiler.hpp)
- [CASTLE core types](../../../../include/castle/core/types.hpp)
- [CASTLE string view](../../../../include/castle/container/string_view.hpp)
- [CASTLE status](../../../../include/castle/error/status.hpp)

## Public API
- `version`: HTTP/1.0, HTTP/1.1, and HTTP/2 identifiers.
- `method`: standard methods GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH, and `custom`.
- `error_code`: stable protocol-level error classification, including malformed messages, bounded-capacity errors, unsupported chunked encoding, HPACK, frame, stream, and flow-control errors.
- `header_view`: non-owning field name/value pair.
- `response_view`: status code, protocol version, fixed header-view range, body view, stream ID, keep-alive flag, and body-streaming indicator.
- `method_token(method)`: returns the standard wire token; custom methods are supplied separately.
- `ascii_iequal`, `ascii_lowercase`: ASCII-only case helpers.
- `to_status(error_code)`: maps protocol errors to `castle::status`.

## Usage Example
```cpp
#include "castle_ext/protocols/http/types.hpp"

using namespace castle::protocols::http;

int main()
{
    response_view response{};
    response.protocol = version::http_2;
    response.status_code = 200U;
    return ascii_iequal(method_token(method::get), string_view("get")) ? 0 : 1;
}
```

## Constraints & Notes
All views are non-owning and remain valid only while the owning parser/session storage is unchanged. Comparisons are byte-wise ASCII operations; locale rules are not used. No allocation or exceptions are used.
