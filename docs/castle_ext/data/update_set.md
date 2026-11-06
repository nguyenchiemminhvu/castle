# UpdateSet

## Overview

`castle_ext::data::update_set<Ts...>` is a fixed-size set that records which data types have changed. It is the notification payload of the data layer. Producers publish values into a [`data_pool`](data_pool.md) and only tell consumers *which* types changed. Consumers then read just those types, so large structures are never copied along the notification path.

Each type in `Ts...` maps to one bit, in declaration order. The set is a plain value: cheap to copy, compare, merge, and filter.

## Header

`#include "castle_ext/data/update_set.hpp"`

## Dependencies

- [`castle/core/compiler.hpp`](../../../include/castle/core/compiler.hpp)
- [`castle/core/traits.hpp`](../../../include/castle/core/traits.hpp)
- [`castle/core/types.hpp`](../../../include/castle/core/types.hpp)
- [`castle/utility/bitset.hpp`](../../utility/bitset.md)
- [`castle/utility/tuple.hpp`](../../utility/tuple.md)

## Public API

| API | Description |
|---|---|
| `update_set<Ts...>` | Set over the unique types `Ts...` (at least one type). |
| `update_set()` | Constructs an empty set. |
| `static of<Us...>()` | Returns a set containing exactly `Us...`. |
| `static all()` | Returns a set containing every type of `Ts...`. |
| `static capacity()` | `constexpr`; returns `sizeof...(Ts)`. |
| `contains<T>()` | Returns whether `T` is in the set. |
| `insert<Us...>()` / `erase<Us...>()` | Add or remove types; return `*this`. |
| `clear()` | Removes every type. |
| `empty()` / `size()` | Returns whether the set is empty / how many types it contains. |
| `operator\|=`, `operator\|` | Union. |
| `operator&=`, `operator&` | Intersection, e.g. to filter by an application's interest. |
| `operator==`, `operator!=` | Compare set contents. |
| `for_each(visitor)` | Calls `visitor(castle::type_identity<T>{})` for every member `T`, in declaration order. |

Naming a type that is not in `Ts...` causes a `static_assert` failure. cv-qualifiers on the queried type are ignored.

## Usage Example

See [`samples/castle_ext/data/update_set.cpp`](../../../samples/castle_ext/data/update_set.cpp).

```cpp
using updates = castle_ext::data::update_set<imu_sample, gnss_fix, battery_state>;

updates changed = updates::of<imu_sample>();
changed |= updates::of<battery_state>();

const updates for_navigation = changed & updates::of<imu_sample, gnss_fix>();   // { imu_sample }

for_navigation.for_each([&](auto type)
{
    using changed_type = typename decltype(type)::type;
    handle(pool.get_snapshot<changed_type>());
});
```

## Constraints & Notes

- Storage is one `castle::bitset<sizeof...(Ts)>` of 32-bit words. Nothing is allocated.
- All operations are O(number of storage words), except `for_each()`, which is O(`sizeof...(Ts)`).
- Types must be unique; duplicates cause a `static_assert` failure.
- The set is not synchronized. To share pending updates between threads, use [`update_inbox`](update_inbox.md); to react to each update with a per-type callback, use [`update_dispatcher`](update_dispatcher.md).
