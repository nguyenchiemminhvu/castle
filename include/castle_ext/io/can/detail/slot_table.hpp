// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file slot_table.hpp
 * @brief Fixed-capacity slot allocator behind every backend's filter bookkeeping.
 *
 * SocketCAN, Zephyr and the mock all need the same thing: hand out a small integer handle for an
 * installed filter, remember what is stored under it, and free it again. `slot_table<T, N>` keeps
 * `N` inline values plus an occupancy bitset; acquire/release are O(N) at most and never allocate.
 */
#ifndef CASTLE_EXT_IO_CAN_DETAIL_SLOT_TABLE_HPP
#define CASTLE_EXT_IO_CAN_DETAIL_SLOT_TABLE_HPP

#include "castle/container/array.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/bitset.hpp"
#include "castle_ext/io/can/filter.hpp"

#include <limits.h>

namespace castle
{
namespace io
{
namespace can
{
namespace detail
{

/**
 * @brief Inline table of `N` slots addressed by `filter_handle`.
 * @tparam T Stored value type; must be default constructible and assignable.
 * @tparam N Number of slots; must be in `[1, INT_MAX]`.
 */
template <typename T, size_type N>
class slot_table
{
    static_assert(N > 0U, "slot_table capacity must be non-zero");
    static_assert(N <= static_cast<size_type>(INT_MAX), "slot_table capacity exceeds the handle range");

public:
    /** @brief Creates an empty table. */
    slot_table() CASTLE_NOEXCEPT
        : items_()
        , used_()
    {
    }

    /** @brief Returns the number of slots. */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /** @brief Returns the number of occupied slots. */
    CASTLE_NODISCARD size_type count() CASTLE_CONST CASTLE_NOEXCEPT { return used_.count(); }

    /** @brief Returns true when `handle` names an occupied slot. */
    CASTLE_NODISCARD bool is_used(filter_handle handle) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return handle >= 0 && static_cast<size_type>(handle) < N && used_.test(static_cast<size_type>(handle));
    }

    /**
     * @brief Stores `value` in the lowest free slot.
     * @return The slot handle, or `invalid_filter` when the table is full.
     */
    CASTLE_NODISCARD filter_handle acquire(T CASTLE_CONST& value) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            if (!used_.test(i))
            {
                items_[i] = value;
                used_.set(i);
                return static_cast<filter_handle>(i);
            }
        }
        return invalid_filter;
    }

    /** @brief Frees a slot. Returns false when `handle` does not name an occupied slot. */
    bool release(filter_handle handle) CASTLE_NOEXCEPT
    {
        if (!is_used(handle))
        {
            return false;
        }
        used_.reset(static_cast<size_type>(handle));
        return true;
    }

    /** @brief Frees every slot. */
    void clear() CASTLE_NOEXCEPT { used_.reset(); }

    /** @brief Returns the value in an occupied slot; `handle` must satisfy `is_used()`. */
    T& at(filter_handle handle) CASTLE_NOEXCEPT { return items_[static_cast<size_type>(handle)]; }

    /** @brief Returns the value in an occupied slot; `handle` must satisfy `is_used()`. */
    T CASTLE_CONST& at(filter_handle handle) CASTLE_CONST CASTLE_NOEXCEPT { return items_[static_cast<size_type>(handle)]; }

private:
    container::array<T, N> items_;
    castle::bitset<N> used_;
};

} // namespace detail
} // namespace can
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_CAN_DETAIL_SLOT_TABLE_HPP
