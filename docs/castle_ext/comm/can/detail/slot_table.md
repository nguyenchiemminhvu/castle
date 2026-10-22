# CAN Slot Table

## Overview
`castle::comm::can::detail::slot_table<T, N>` is the fixed-capacity slot allocator behind every CAN backend's filter bookkeeping. SocketCAN, Zephyr and the mock all need the same thing: hand out a small integer handle for an installed filter, remember what is stored under it, and free it again. The table keeps `N` inline values plus an occupancy bitset; acquire and release are O(N) at most and never allocate. Handles are `filter_handle` values in `[0, N)`, and `invalid_filter` (`-1`) signals "no slot".

This is an implementation detail in namespace `detail`: applications use filters through `basic_bus` and do not need it, but custom backends can reuse it.

## Header
`#include "castle_ext/comm/can/detail/slot_table.hpp"`

## Dependencies
- [`../../../../container/array.md`](../../../../container/array.md)
- [`../../../../core/compiler.md`](../../../../core/compiler.md)
- [`../../../../core/types.md`](../../../../core/types.md)
- [`../../../../utility/bitset.md`](../../../../utility/bitset.md)
- [`../filter.md`](../filter.md) (`filter_handle`, `invalid_filter`)

## Public API
Template parameters: `T` is the stored value type (default constructible and assignable); `N` is the number of slots and must be in `[1, INT_MAX]`, both enforced with `static_assert`.

| API | Description |
|---|---|
| `slot_table()` | Creates an empty table (values default constructed, all slots free). `noexcept`. |
| `static capacity()` | `constexpr size_type`, returns `N`. |
| `count() const` | Number of occupied slots. `noexcept`, linear in `N`. |
| `is_used(filter_handle handle) const` | True when `handle` is within `[0, N)` and names an occupied slot. Any other value, including `invalid_filter`, returns false. `noexcept`, O(1). |
| `acquire(T const& value)` | Copies `value` into the **lowest** free slot and returns its handle, or `invalid_filter` when the table is full. Marked `CASTLE_NODISCARD`. `noexcept`, O(N). |
| `release(filter_handle handle)` | Frees an occupied slot. Returns `true`, or `false` when `handle` does not name an occupied slot (nothing changes). `noexcept`, O(1). |
| `clear()` | Frees every slot. Stored values are left in place but become unreachable. `noexcept`. |
| `at(filter_handle handle)` / `at(filter_handle) const` | Reference to the value in a slot. **Unchecked**: `handle` must satisfy `is_used()`. `noexcept`, O(1). |

## Usage Example
See `samples/castle_ext/comm/can/detail/slot_table.cpp`.

```cpp
#include "castle_ext/comm/can/detail/slot_table.hpp"

#include <assert.h>

using namespace castle::comm::can;

int main()
{
    detail::slot_table<filter, 3U> table;
    assert(table.capacity() == 3U);

    const filter_handle first = table.acquire(filter::exact(0x100U));
    const filter_handle second = table.acquire(filter::any(id_format::extended));
    assert(first == 0 && second == 1);
    assert(table.count() == 2U);
    assert(table.at(second).format == id_format::extended);

    assert(table.release(first));
    assert(!table.release(first));                 // already free
    assert(!table.is_used(invalid_filter));

    const filter_handle reused = table.acquire(filter::exact(0x200U));
    assert(reused == first);                       // lowest free slot is reused

    assert(table.acquire(filter()) == 2);
    assert(table.acquire(filter()) == invalid_filter);   // full
    table.clear();
    assert(table.count() == 0U);
    return 0;
}
```

## Constraints & Notes
- Storage is `container::array<T, N>` plus `castle::bitset<N>`: no heap, exceptions, RTTI or virtual functions. The table is as large as `N * sizeof(T)` plus the bitset.
- Handles are stable while a slot is occupied and are reused after `release()`/`clear()`, always lowest-first. A stale handle can therefore name a newer filter; backends only hand handles to the application through `add_filter()` and treat unknown values with `is_used()` (reporting `not_found`).
- The table has no locking. Backends that mutate it from several contexts must serialise access themselves (the SocketCAN backend documents that filter changes need external synchronisation).
- Zephyr stores the driver's filter id (`int`) as `T`, SocketCAN and the mock store the `filter` itself.
- Not part of the stable public API: it lives in `detail` and may change with the backends.
- Related: [`../filter.md`](../filter.md), [`../bus.md`](../bus.md), [`../can.md`](../can.md).
