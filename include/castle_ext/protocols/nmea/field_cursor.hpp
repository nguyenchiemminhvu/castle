// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file field_cursor.hpp
 * @brief Sequential field reader over a tokenized NMEA sentence for Castle.
 *
 * NMEA decoders index `message_view::field(i)` directly, which is easy to
 * get wrong by one position when a sentence has many fields (this exact
 * mistake was made once while testing the sentence generator - see repo
 * notes). `field_cursor` removes the manual index bookkeeping: each `next_*()`
 * call consumes the current field, advances, and parses in one step.
 *
 * Unlike `ubx::payload_reader`'s sticky-error model, NMEA fields are
 * routinely and legitimately empty (e.g. HDOP absent without a fix), so this
 * cursor does not latch a global failure flag. Each call reports its own
 * success; callers combine mandatory fields with `&&` and read optional
 * fields with `next_double_or()`.
 *
 * @code
 * castle::protocols::nmea::field_cursor cursor(sentence);
 * double utc_time = 0.0;
 * bool ok = cursor.next_double(utc_time);
 * double lat = 0.0;
 * ok = ok && cursor.next_latlon(lat);
 * double hdop = cursor.next_double_or(0.0); // optional, defaults when empty
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_NMEA_FIELD_CURSOR_HPP
#define CASTLE_EXT_PROTOCOLS_NMEA_FIELD_CURSOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string_view.hpp"

#include "castle_ext/protocols/nmea/nmea.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace nmea
{

/**
 * @brief Auto-advancing reader over one tokenized sentence's fields.
 *
 * @warning The referenced `message_view` (and the buffers it aliases) must
 * outlive the cursor, matching the view's own zero-copy lifetime rules.
 */
class field_cursor CASTLE_FINAL
{
public:
    CASTLE_CONSTEXPR explicit field_cursor(CASTLE_CONST message_view& sentence) CASTLE_NOEXCEPT
        : sentence_(sentence)
    {
    }

    /** @brief Returns the current field and advances past it. */
    CASTLE_NODISCARD castle::container::string_view next() CASTLE_NOEXCEPT
    {
        CASTLE_CONST castle::container::string_view value = sentence_.field(index_);
        ++index_;
        return value;
    }

    /** @brief Index of the field that the next `next_*()` call will consume. */
    CASTLE_NODISCARD castle::size_type index() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index_;
    }

    /** @brief Advances past `count` fields without parsing them. */
    void skip(castle::size_type count = 1U) CASTLE_NOEXCEPT
    {
        index_ += count;
    }

    CASTLE_NODISCARD bool next_double(double& out) CASTLE_NOEXCEPT
    {
        return parse_double(next(), out);
    }

    CASTLE_NODISCARD bool next_int(int& out, int base = 10) CASTLE_NOEXCEPT
    {
        return parse_int(next(), out, base);
    }

    CASTLE_NODISCARD bool next_uint(uint32_t& out, int base = 10) CASTLE_NOEXCEPT
    {
        return parse_uint(next(), out, base);
    }

    CASTLE_NODISCARD bool next_char(char& out) CASTLE_NOEXCEPT
    {
        return parse_char(next(), out);
    }

    /** @brief Consumes a value field followed by its N/S or E/W direction field. */
    CASTLE_NODISCARD bool next_latlon(double& out) CASTLE_NOEXCEPT
    {
        CASTLE_CONST castle::container::string_view value = next();
        CASTLE_CONST castle::container::string_view direction = next();
        return parse_latlon(value, direction, out);
    }

    /** @brief Consumes one optional numeric field, keeping `fallback` when empty or unparsable. */
    CASTLE_NODISCARD double next_double_or(double fallback) CASTLE_NOEXCEPT
    {
        double value = fallback;
        CASTLE_CONST castle::container::string_view field = next();
        if (!field.empty()) // LCOV_EXCL_BR_LINE
        {
            // An unparsable non-empty field keeps `fallback`; nothing else to do with the result.
            static_cast<void>(parse_double(field, value));
        }
        return value;
    }

    /** @brief Consumes one optional unsigned field, keeping `fallback` when empty or unparsable. */
    CASTLE_NODISCARD uint32_t next_uint_or(uint32_t fallback, int base = 10) CASTLE_NOEXCEPT
    {
        uint32_t value = fallback;
        CASTLE_CONST castle::container::string_view field = next();
        if (!field.empty()) // LCOV_EXCL_BR_LINE
        {
            // An unparsable non-empty field keeps `fallback`; nothing else to do with the result.
            static_cast<void>(parse_uint(field, value, base));
        }
        return value;
    }

private:
    CASTLE_CONST message_view& sentence_;
    castle::size_type index_ = 0U;
};

} // namespace nmea
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_NMEA_FIELD_CURSOR_HPP
