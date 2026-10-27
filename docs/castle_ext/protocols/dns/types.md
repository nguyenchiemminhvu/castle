# DNS Types

## Overview
Defines the fixed-capacity value types shared by the DNS message codec and resolver: query types, response codes, resource records, parsed responses, and timeout/retry configuration.

## Header
`#include "castle_ext/protocols/dns/types.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/container/array.hpp`](../../../../include/castle/container/array.hpp)
- [`castle_ext/net/endpoint.hpp`](../../../../include/castle_ext/net/endpoint.hpp)

## Public API

| Type | Description |
|---|---|
| `record_type` | DNS types `a`, `ns`, `cname`, `mx`, and `aaaa`. |
| `record_class` | Internet class (`internet`). |
| `section` | `answer`, `authority`, or `additional` section. |
| `response_code` | RFC 1035 response codes 0–5. |
| `resource_record` | One decoded RR, with fixed owner/target names, TTL, optional IP endpoint, and MX preference. |
| `response<MaxRecords>` | Parsed flags/counts/question plus at most `MaxRecords` supported RRs; `clear()` resets it. |
| `client_config` | Per-attempt timeout, UDP retransmission count, and transaction-ID seed. |

## Usage Example

```cpp
#include "castle_ext/protocols/dns/types.hpp"

castle::protocols::dns::response<8U> answer;
castle::protocols::dns::client_config config;
config.timeout_ms = 1500U;
config.retries = 1U;
```

See [`samples/castle_ext/protocols/dns/types.cpp`](../../../../samples/castle_ext/protocols/dns/types.cpp).

## Constraints & Notes
The RR collection uses `castle::container::array`; all other storage is inline and fixed-capacity. `response<N>` retains supported records from all three RR sections and reports `full` if more than `N` supported records are present. Names use 256-byte null-terminated arrays. Values are plain data and require no initialization service or heap allocation.
