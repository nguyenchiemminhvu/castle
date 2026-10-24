#include "sample_support.hpp"

#include "castle_ext/io/can/detail/slot_table.hpp"

using namespace castle::io::can;

int main()
{
    // Backends keep installed filters in a slot_table; the slot index is the public filter_handle.
    detail::slot_table<filter, 3U> table;
    static_assert(detail::slot_table<filter, 3U>::capacity() == 3U, "capacity is a compile-time constant");
    CASTLE_SAMPLE_CHECK(table.capacity() == 3U);
    CASTLE_SAMPLE_CHECK(table.count() == 0U);
    CASTLE_SAMPLE_CHECK(!table.is_used(0));

    // acquire() hands out the lowest free slot.
    const filter_handle first = table.acquire(filter::exact(0x100U));
    const filter_handle second = table.acquire(filter::exact(0x200U));
    const filter_handle third = table.acquire(filter::any(id_format::extended));
    CASTLE_SAMPLE_CHECK(first == 0 && second == 1 && third == 2);
    CASTLE_SAMPLE_CHECK(table.count() == 3U);
    CASTLE_SAMPLE_CHECK(table.is_used(second));

    // A full table refuses further values.
    CASTLE_SAMPLE_CHECK(table.acquire(filter::exact(0x300U)) == invalid_filter);
    CASTLE_SAMPLE_CHECK(table.count() == 3U);

    // at() returns the stored value, mutable or const.
    CASTLE_SAMPLE_CHECK(table.at(first).id == 0x100U);
    table.at(first).id = 0x101U;
    const detail::slot_table<filter, 3U>& view = table;
    CASTLE_SAMPLE_CHECK(view.at(first).id == 0x101U);
    CASTLE_SAMPLE_CHECK(view.at(third).format == id_format::extended);

    // release() frees one slot; the next acquire() reuses the lowest freed index.
    CASTLE_SAMPLE_CHECK(table.release(second));
    CASTLE_SAMPLE_CHECK(!table.is_used(second));
    CASTLE_SAMPLE_CHECK(table.count() == 2U);
    CASTLE_SAMPLE_CHECK(!table.release(second)); // already free
    CASTLE_SAMPLE_CHECK(table.acquire(filter::exact(0x400U)) == second);
    CASTLE_SAMPLE_CHECK(table.at(second).id == 0x400U);

    // Handles outside the table are never used and never released.
    CASTLE_SAMPLE_CHECK(!table.is_used(-1));
    CASTLE_SAMPLE_CHECK(!table.is_used(3));
    CASTLE_SAMPLE_CHECK(!table.release(invalid_filter));
    CASTLE_SAMPLE_CHECK(!table.release(42));

    // clear() frees everything at once.
    table.clear();
    CASTLE_SAMPLE_CHECK(table.count() == 0U);
    CASTLE_SAMPLE_CHECK(!table.is_used(first) && !table.is_used(third));
    CASTLE_SAMPLE_CHECK(table.acquire(filter::any()) == 0);

    // Other backends reuse the table for driver-side identifiers (Zephyr stores ints).
    detail::slot_table<int, 2U> driver_ids;
    const filter_handle slot = driver_ids.acquire(17);
    CASTLE_SAMPLE_CHECK(slot == 0 && driver_ids.at(slot) == 17);
    CASTLE_SAMPLE_CHECK(driver_ids.release(slot));

    return 0;
}
