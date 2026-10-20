// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_timeutc.hpp
 * @brief UBX-NAV-TIMEUTC (0x01 0x21): UTC time solution.
 *
 * Periodic/polled output. During a leap second `sec` may read 60.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEUTC_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEUTC_HPP

#include "castle/core/compiler.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/detail/fixed_message.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief `utcStandard` (bits 7..4 of `valid`): UTC standard identifier. */
enum class nav_timeutc_utc_standard : uint8_t
{
    not_available = 0U,        ///< Information not available.
    crl = 1U,                  ///< Communications Research Laboratory (CRL), Tokyo, Japan.
    nist = 2U,                 ///< National Institute of Standards and Technology.
    usno = 3U,                 ///< U.S. Naval Observatory.
    bipm = 4U,                 ///< International Bureau of Weights and Measures.
    european_laboratories = 5U,
    former_soviet_union = 6U,
    ntsc = 7U,                 ///< National Time Service Center, China.
    unknown = 15U
};

/** @brief UBX-NAV-TIMEUTC output payload. */
struct nav_timeutc : detail::fixed_message<nav_timeutc, UBX_CLASS_NAV, UBX_ID_NAV_TIMEUTC, 20U>
{
    /** @name `valid` bits @{ */
    static CASTLE_CONSTEXPR uint8_t VALID_TOW = 0x01U;                ///< `validTOW`.
    static CASTLE_CONSTEXPR uint8_t VALID_WKN = 0x02U;                ///< `validWKN`.
    static CASTLE_CONSTEXPR uint8_t VALID_UTC = 0x04U;                ///< `validUTC`.
    static CASTLE_CONSTEXPR uint8_t VALID_UTC_STANDARD_MASK = 0xF0U;  ///< `utcStandard`.
    static CASTLE_CONSTEXPR uint8_t VALID_UTC_STANDARD_SHIFT = 4U;
    /** @} */

    uint32_t i_tow = 0U;  ///< GPS time of week of the navigation epoch [ms].
    uint32_t t_acc = 0U;  ///< UTC time accuracy estimate [ns].
    int32_t nano = 0;     ///< Fraction of second, -1e9..1e9 [ns].
    uint16_t year = 0U;   ///< Year, 1999..2099 [y].
    uint8_t month = 0U;   ///< Month, 1..12.
    uint8_t day = 0U;     ///< Day of month, 1..31 [d].
    uint8_t hour = 0U;    ///< Hour of day, 0..23 [h].
    uint8_t min = 0U;     ///< Minute of hour, 0..59 [min].
    uint8_t sec = 0U;     ///< Seconds of minute, 0..60 [s].
    uint8_t valid = 0U;   ///< Validity flags and UTC standard (`VALID_*`).

    CASTLE_NODISCARD bool valid_tow() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_TOW) != 0U;
    }

    CASTLE_NODISCARD bool valid_wkn() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_WKN) != 0U;
    }

    CASTLE_NODISCARD bool valid_utc() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_UTC) != 0U;
    }

    /** @brief `utcStandard`; values the spec does not list are returned unchanged. */
    CASTLE_NODISCARD nav_timeutc_utc_standard utc_standard() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_timeutc_utc_standard>(
            static_cast<uint8_t>(valid & VALID_UTC_STANDARD_MASK) >> VALID_UTC_STANDARD_SHIFT);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.t_acc);
        stream.field(self.nano);
        stream.field(self.year);
        stream.field(self.month);
        stream.field(self.day);
        stream.field(self.hour);
        stream.field(self.min);
        stream.field(self.sec);
        stream.field(self.valid);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEUTC_HPP
