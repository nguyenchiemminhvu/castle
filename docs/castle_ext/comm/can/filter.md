# CAN Filter

## Overview
`castle::comm::can::filter` is the identifier/mask acceptance filter shared by every CAN backend. A frame passes a filter when it has the filter's identifier format and `((frame.id ^ filter.id) & filter.mask) == 0`; a zero mask bit is "don't care". This is the model of CAN controller hardware filters, of `struct can_filter` on SocketCAN and of `struct can_filter` on Zephyr, so a filter maps one-to-one onto each platform and onto the mock backend.

The header also defines `filter_handle`, the integer that names an installed filter, `invalid_filter`, and `validate_filter()`.

## Header
`#include "castle_ext/comm/can/filter.hpp"`

## Dependencies
- [`../../../core/compiler.md`](../../../core/compiler.md)
- [`../../../error/status.md`](../../../error/status.md)
- [`frame.md`](frame.md)

## Public API
All names live in `castle::comm::can`.

| API | Description |
|---|---|
| `filter_handle` | `int`. Names one installed filter; returned by `basic_bus::add_filter()` and consumed by `remove_filter()`. Valid handles are `0 .. MaxFilters-1` of the backend. |
| `invalid_filter` | `filter_handle` constant, `-1`. Never names an installed filter. |
| `struct filter` | Public fields `uint32_t id`, `uint32_t mask`, `id_format format`. |
| `filter()` | `constexpr`. `id = 0`, `mask = 0`, standard format: accepts every standard-format frame. |
| `filter(uint32_t identifier, uint32_t identifier_mask, id_format id_kind = id_format::standard)` | `constexpr`. Masked filter; only bits set in the mask are compared. No range check (see `validate_filter()`). |
| `static filter::exact(uint32_t identifier, id_format id_kind = id_format::standard)` | `constexpr`. Accepts exactly one identifier: mask is `max_id(id_kind)` (all identifier bits). |
| `static filter::any(id_format id_kind = id_format::standard)` | `constexpr`. Accepts every frame of that format: identifier `0`, mask `0`. |
| `matches(frame const&) const` | `constexpr bool`. True when `candidate.format == format && ((candidate.id ^ id) & mask) == 0`. `noexcept`, O(1). |
| `validate_filter(filter const&)` | Returns `ok`, or `invalid_argument` for an unknown `format` enumerator or an `id` or `mask` above `max_id(format)`. `basic_bus::add_filter()` calls it first. |

## Usage Example
See `samples/castle_ext/comm/can/filter.cpp`.

```cpp
#include "castle_ext/comm/can/filter.hpp"

#include <assert.h>

using namespace castle::comm::can;

int main()
{
    const filter one_id = filter::exact(0x123U);
    const filter one_block(0x120U, 0x7F0U);                // 0x120 .. 0x12F
    const filter every_extended = filter::any(id_format::extended);

    assert(one_id.matches(frame(0x123U)));
    assert(!one_id.matches(frame(0x124U)));
    assert(one_block.matches(frame(0x12FU)));
    assert(!one_block.matches(frame(0x130U)));

    // The format must match: standard 0x123 and extended 0x123 are different frames.
    assert(!one_id.matches(frame(0x123U, id_format::extended)));
    assert(every_extended.matches(frame(0x1ABCDEF0U, id_format::extended)));

    // Data and remote frames both pass.
    frame request(0x123U);
    assert(request.set_remote(8U) == castle::status::ok);
    assert(one_id.matches(request));

    assert(validate_filter(filter(0x800U, 0x7FFU)) == castle::status::invalid_argument);
    assert(validate_filter(filter(0x800U, 0x7FFU, id_format::extended)) == castle::status::ok);
    return 0;
}
```

## Constraints & Notes
- `filter` is a trivially copyable value of three fields: no heap, exceptions, RTTI or virtual functions.
- Filtering is by identifier and format only. Remote and data frames both pass; test `frame::is_remote()` when you need to tell them apart.
- A freshly opened controller has **no** filter installed and therefore receives nothing; install `filter::any()` (or `basic_bus::accept_all()`, which installs one `any` filter per format) to receive everything.
- A backend holds a fixed number of filters (`MaxFilters` template argument; default 8 for the mock, SocketCAN and Zephyr backends). A full table is reported by `add_filter()` as `full`. Hardware may offer fewer slots than the template allows (the stock MCP2515 Zephyr driver has five).
- On SocketCAN the extended-format flag is always part of the kernel mask, so standard and extended filters never match each other's frames. The Zephyr backend uses the `CAN_FILTER_IDE` flag for the same purpose.
- When filters overlap, the Zephyr backend delivers a frame once per matching filter; the mock backend delivers it once.
- Related: [`frame.md`](frame.md), [`bus.md`](bus.md), [`detail/slot_table.md`](detail/slot_table.md), [`can.md`](can.md).
