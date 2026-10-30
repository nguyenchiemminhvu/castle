# CoAP Types

## Overview
Defines the fixed-capacity message, message-code, option-number, and option-value model used by CASTLE's CoAP module. It also provides URI path/query construction and URI path matching helpers.

## Header
`#include "castle_ext/protocols/coap/types.hpp"`

## Dependencies
- [Core compiler macros](../../../../include/castle/core/compiler.hpp)
- [Core types](../../../../include/castle/core/types.hpp)
- [Status](../../../../include/castle/error/status.hpp)

## Public API

| API | Description |
|---|---|
| `message_type` | CON, NON, ACK, and RST message types. |
| `code` | Empty, GET/POST/PUT/DELETE, requested 2.xx/4.xx/5.xx response codes. Unknown wire codes remain representable in `code_value`. |
| `option_number` | Supported RFC 7252 options plus Observe, Block1/2, and Size2. |
| `message<MaxOptions, OptionValueCapacity, PayloadCapacity>` | Complete bounded message with `set_token`, `set_payload`, `append_option`, `append_uint_option`, `append_string_option`, and `find_option`. |
| `is_critical_option`, `is_unsafe_option`, `is_no_cache_key_option` | Option-number classification helpers. |
| `append_uri_path(message, path)` | Splits a slash-delimited path into repeated Uri-Path options. |
| `append_uri_query(message, query)` | Splits ampersand-separated query components into Uri-Query options. |
| `uri_path_matches(message, path)` | Exact hierarchical Uri-Path comparison. |

## Usage Example

```cpp
#include "castle_ext/protocols/coap/types.hpp"

int main()
{
    castle::protocols::coap::message<> request;
    request.type = castle::protocols::coap::message_type::confirmable;
    request.code_value = static_cast<uint8_t>(castle::protocols::coap::code::get);
    request.message_id = 7U;
    const uint8_t token[] = {0xA1U, 0x01U};
    if (request.set_token(token, sizeof(token)) != castle::status::ok) return 1;
    if (castle::protocols::coap::append_uri_path(request, "/sensors/temperature") != castle::status::ok) return 2;
    return castle::protocols::coap::uri_path_matches(request, "sensors/temperature") ? 0 : 3;
}
```

## Constraints & Notes
All storage is inline. Defaults are 16 options, 64 bytes per option value, and 1024 payload bytes. Repeated options are inserted in option-number order. URI strings are accepted as already percent-encoded components; this helper does not parse full CoAP URIs or percent-decode them. No STL, heap allocation, exceptions, RTTI, or virtual dispatch is used.
