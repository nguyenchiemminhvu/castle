# UBX repeated_group

## Overview

Many UBX payloads are a fixed header followed by `N` equally sized blocks, such as `8 + numSvs·12` (NAV-SAT) or `4 + nBlocks·24` (MON-RF). `repeated_group.hpp` handles the two recurring steps once:

- validating the payload size (`has_group_length()`, `implied_group_count()`);
- walking the blocks with the same layout verbs as [`fixed_message`](fixed_message.md) (`read_group()` / `write_group()`), plus free-length byte tails (`read_tail()` / `write_tail()`).

Blocks are stored in a fixed-capacity [`castle::container::vector`](../../../../../container/vector.md), whose `size()` is the number actually stored. Blocks beyond its capacity are skipped, so the cursor always ends on the next field. The message's spec-named count field (e.g. `num_svs`) still reports the on-wire count.

## Header

`#include "castle_ext/protocols/ubx/messages/detail/repeated_group.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../../../core/compiler.md)
- [`castle/core/types.hpp`](../../../../../core/types.md)
- [`castle/container/vector.hpp`](../../../../../container/vector.md)
- [`castle_ext/protocols/ubx/payload_reader.hpp`](../../payload_reader.md)
- [`castle_ext/protocols/ubx/payload_writer.hpp`](../../payload_writer.md)

## Public API

| Function | Description |
| --- | --- |
| `constexpr bool has_group_length(payload_length, header_length, count, block_size)` | `payload_length == header_length + count * block_size`. |
| `constexpr bool implied_group_count(payload_length, header_length, block_size, size_type& count_out)` | For groups without a count field (MON-IO, ESF-RAW, MON-VER extensions). Sets `count_out` to the number of whole blocks after the header. Returns `false` (and `count_out = 0`) when the payload is shorter than the header or the remainder is not a whole number of blocks. |
| `void read_group(payload_reader&, size_type wire_count, vector<Block, N>& out)` | Clears `out` and decodes up to `N` blocks with `Block::layout`. The rest are skipped, so the cursor advances by exactly `wire_count * Block::wire_size`. |
| `void write_group(payload_writer&, const vector<Block, N>& blocks)` | Encodes every stored block in order. |
| `void read_tail(payload_reader&, vector<T, N>& out)` | Stores up to `N` remaining bytes (byte-sized `T` only, enforced by `static_assert`) and skips the rest. |
| `void write_tail(payload_writer&, const vector<T, N>& bytes)` | Writes every stored byte. |

### Block requirements

```cpp
struct block
{
    static constexpr castle::size_type wire_size = /* bytes per block */;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) noexcept;
};
```

`Block` must be default-constructible and copyable, and its layout must consume exactly `wire_size` bytes.

## Usage Example

```cpp
// Inside a variable-length message's decode(), after reading the header:
if (!detail::has_group_length(raw.payload_length(), header_length, out.num_fences, block_length))
{
    out = nav_geofence{};
    return false;
}
detail::read_group(reader, out.num_fences, out.fences);
return reader.ok();
```

See [`nav_geofence`](../nav_geofence.md) for the complete pattern, including `encode()`.

## Constraints & Notes

- No heap allocation; storage is the caller's fixed-capacity `vector`.
- Complexity: O(wire_count) for `read_group`, O(size()) for the write helpers.
- Read failures latch into the reader (`reader.ok()`); the calling decoder checks once.
- Validate the length **before** calling `read_group` so a wrong count can never make the cursor skip past the payload.
