# HPACK Codec

## Overview
Provides bounded RFC 7541 HPACK decoding (static and optional dynamic table, including the Huffman alphabet) and a minimal valid literal encoder for HTTP/2 requests. The encoder deliberately avoids Huffman and dynamic indexing so its behavior is deterministic and its output does not require mutable compression state.

## Header
`#include "castle_ext/protocols/http/hpack.hpp"`

## Dependencies
- [HTTP shared types](types.md)
- [CASTLE compiler annotations](../../../../include/castle/core/compiler.hpp)
- [CASTLE core types](../../../../include/castle/core/types.hpp)
- [CASTLE status](../../../../include/castle/error/status.hpp)

## Public API
- `hpack_decoder<MaxDynamicTableBytes, MaxStringSize, MaxDynamicEntries>::decode_block(input, size, output)`: decodes one header block into a compatible bounded header collection.
- `dynamic_table_size`, `dynamic_entry_count`, `maximum_table_size`: inspect decoder state.
- `clear_dynamic_table()`: clears dynamic entries while retaining the configured maximum.
- `hpack_encoder::encode_literal(name, value, output, capacity, used)`: writes one literal-without-indexing field using non-Huffman strings.

## Usage Example
```cpp
#include "castle_ext/protocols/http/hpack.hpp"
#include "castle_ext/protocols/http/headers.hpp"

int main()
{
    const uint8_t indexed_get[] = {0x82U}; // Static-table entry 2: :method = GET
    castle::protocols::http::basic_headers<4U, 32U, 64U> headers;
    castle::protocols::http::hpack_decoder<> decoder;
    if (decoder.decode_block(indexed_get, sizeof(indexed_get), headers) != castle::status::ok) { return 1; }
    return headers.get(":method") == "GET" ? 0 : 2;
}
```

## Constraints & Notes
All decoder strings and dynamic entries are held in template-sized arrays. Set `MaxDynamicTableBytes` to zero to disable dynamic table storage; HTTP/2 sessions using that policy advertise `SETTINGS_HEADER_TABLE_SIZE = 0`. `MaxStringSize` bounds each decoded name/value and `MaxDynamicEntries` bounds entry count. The encoder emits non-Huffman literal fields and does not modify a dynamic table. Decode time is linear in encoded bytes plus fixed-table lookup work. No allocation or exceptions.
