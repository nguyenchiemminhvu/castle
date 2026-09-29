// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file schedule.hpp
 * @brief Fixed-capacity LIN master schedule table.
 */
#ifndef CASTLE_EXT_PROTOCOLS_LIN_SCHEDULE_HPP
#define CASTLE_EXT_PROTOCOLS_LIN_SCHEDULE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/protocols/lin/frame.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace lin
{

/** @brief One scheduled LIN header slot. Repeated identifiers are permitted. */
struct schedule_entry
{
    uint8_t identifier = 0U;
    response_owner owner = response_owner::master;
    uint32_t slot_ticks = 1U;

    CASTLE_NODISCARD bool valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_valid_identifier(identifier)
            && (owner == response_owner::master || owner == response_owner::slave)
            && slot_ticks != 0U;
    }
};

/**
 * @brief Bounded cyclic schedule storage for a LIN commander/master.
 * @tparam Capacity Maximum number of slots stored inline; must be non-zero.
 *
 * `next()` advances a cursor but does not wait or access a clock. The caller
 * uses `slot_ticks` with its deterministic timer/tick source and calls the
 * master's `transmit_slot()` to emit each scheduled header.
 */
template <castle::size_type Capacity>
class schedule_table CASTLE_FINAL
{
    static_assert(Capacity > 0U, "LIN schedule_table capacity must be non-zero");

public:
    schedule_table() CASTLE_NOEXCEPT : entries_{}, size_(0U), cursor_(0U) {}

    /** @brief Number of entries currently stored. O(1). */
    CASTLE_NODISCARD castle::size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    /** @brief Compile-time storage capacity. O(1). */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR castle::size_type capacity() CASTLE_NOEXCEPT
    {
        return Capacity;
    }
    /** @brief Whether the table has no configured slots. O(1). */
    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /** @brief Appends a slot; returns `full` when inline capacity is exhausted. */
    castle::status add(uint32_t identifier, response_owner owner, uint32_t slot_ticks = 1U) CASTLE_NOEXCEPT
    {
        if (!is_valid_identifier(identifier)
            || (owner != response_owner::master && owner != response_owner::slave)
            || slot_ticks == 0U)
        {
            return castle::status::invalid_argument;
        }
        schedule_entry candidate{};
        candidate.identifier = static_cast<uint8_t>(identifier);
        candidate.owner = owner;
        candidate.slot_ticks = slot_ticks;
        if (!candidate.valid())
        {
            return castle::status::invalid_argument;
        }
        if (size_ >= Capacity)
        {
            return castle::status::full;
        }
        entries_[size_] = candidate;
        ++size_;
        return castle::status::ok;
    }

    /** @brief Copies an entry by index; output is unchanged for an invalid index. */
    castle::status get(castle::size_type index, schedule_entry& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return castle::status::out_of_range;
        }
        output = entries_[index];
        return castle::status::ok;
    }

    /** @brief Returns the next slot and wraps at the end; an empty table returns `empty`. */
    castle::status next(schedule_entry& output) CASTLE_NOEXCEPT
    {
        if (size_ == 0U)
        {
            return castle::status::empty;
        }
        output = entries_[cursor_];
        ++cursor_;
        if (cursor_ >= size_)
        {
            cursor_ = 0U;
        }
        return castle::status::ok;
    }

    /** @brief Rewinds the cyclic cursor without changing configured entries. */
    void reset() CASTLE_NOEXCEPT { cursor_ = 0U; }

    /** @brief Removes all entries and resets the cursor. O(1). */
    void clear() CASTLE_NOEXCEPT
    {
        size_ = 0U;
        cursor_ = 0U;
    }

private:
    schedule_entry entries_[Capacity];
    castle::size_type size_;
    castle::size_type cursor_;
};

} // namespace lin
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_LIN_SCHEDULE_HPP
