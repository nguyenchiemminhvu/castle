// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file filter.hpp
 * @brief Identifier/mask receive filter shared by every CAN backend.
 *
 * A frame passes a filter when it has the filter's identifier format and
 * `(frame.id & mask) == (filter.id & mask)`: a zero mask bit is "don't care". This is the model
 * of hardware acceptance filters, `struct can_filter` on SocketCAN and `struct can_filter` on
 * Zephyr. Filters select by identifier and format only; data and remote frames both pass, so
 * check `frame::is_remote()` when the distinction matters.
 *
 * @code
 * using namespace castle::comm::can;
 * filter one_id = filter::exact(0x123U);                                // only 0x123
 * filter one_block = filter(0x120U, 0x7F0U);                            // 0x120..0x12F
 * filter every_extended = filter::any(id_format::extended);
 * @endcode
 */
#ifndef CASTLE_EXT_COMM_CAN_FILTER_HPP
#define CASTLE_EXT_COMM_CAN_FILTER_HPP

#include "castle/core/compiler.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/comm/can/frame.hpp"

#include <stdint.h>

namespace castle
{
namespace comm
{
namespace can
{

/** @brief Identifies one installed filter; returned by `add_filter()` and consumed by `remove_filter()`. */
using filter_handle = int;

/** @brief Value that never names an installed filter. */
static CASTLE_INLINE CASTLE_CONSTEXPR filter_handle invalid_filter = -1;

/** @brief Identifier/mask acceptance filter. */
struct filter
{
    uint32_t id;
    uint32_t mask;
    id_format format;

    /** @brief Creates a filter that accepts every standard-format frame. */
    CASTLE_CONSTEXPR filter() CASTLE_NOEXCEPT
        : id(0U)
        , mask(0U)
        , format(id_format::standard)
    {
    }

    /**
     * @brief Creates a masked filter.
     * @param identifier Identifier to compare against.
     * @param identifier_mask Bits that must match; zero bits are ignored.
     * @param id_kind Identifier format a frame must have.
     */
    CASTLE_CONSTEXPR filter(uint32_t identifier,
                            uint32_t identifier_mask,
                            id_format id_kind = id_format::standard) CASTLE_NOEXCEPT
        : id(identifier)
        , mask(identifier_mask)
        , format(id_kind)
    {
    }

    /** @brief Creates a filter that accepts exactly one identifier. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR filter exact(uint32_t identifier,
                                                          id_format id_kind = id_format::standard) CASTLE_NOEXCEPT
    {
        return filter(identifier, max_id(id_kind), id_kind);
    }

    /** @brief Creates a filter that accepts every frame of the given format. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR filter any(id_format id_kind = id_format::standard) CASTLE_NOEXCEPT
    {
        return filter(0U, 0U, id_kind);
    }

    /** @brief Returns true when `candidate` passes this filter. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool matches(frame CASTLE_CONST& candidate) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return candidate.format == format && ((candidate.id ^ id) & mask) == 0U;
    }
};

/**
 * @brief Checks that a filter's identifier and mask fit its format.
 * @return `ok`, or `invalid_argument` for an unknown format or an identifier/mask wider than the format allows.
 */
CASTLE_NODISCARD CASTLE_INLINE castle::status validate_filter(filter CASTLE_CONST& value) CASTLE_NOEXCEPT
{
    if ((value.format != id_format::standard && value.format != id_format::extended)
        || value.id > max_id(value.format)
        || value.mask > max_id(value.format))
    {
        return castle::status::invalid_argument;
    }
    return castle::status::ok;
}

} // namespace can
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_CAN_FILTER_HPP
