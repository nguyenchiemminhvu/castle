// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_gnss_time.hpp
 * @brief Shared 20-byte layout of UBX-NAV-TIMEBDS, UBX-NAV-TIMEGAL and UBX-NAV-TIMEQZSS.
 *
 * The three messages are byte-identical; only the spec names differ:
 *
 * | offset | type | generic   | TIMEBDS   | TIMEGAL      | TIMEQZSS      |
 * | ------ | ---- | --------- | --------- | ------------ | ------------- |
 * | 0      | U4   | `i_tow`   | iTOW      | iTOW         | iTOW          |
 * | 4      | U4   | `tow`     | SOW       | galTow       | qzssTow       |
 * | 8      | I4   | `f_tow`   | fSOW      | fGalTow      | fQzssTow      |
 * | 12     | I2   | `week`    | week      | galWno       | qzssWno       |
 * | 14     | I1   | `leap_s`  | leapS     | leapS        | leapS         |
 * | 15     | X1   | `valid`   | valid     | valid        | valid         |
 * | 16     | U4   | `t_acc`   | tAcc      | tAcc         | tAcc          |
 *
 * The layout is written once here; each message is a named struct deriving
 * from `nav_gnss_time<Self, Id>` that adds read-only accessors carrying its
 * spec names (e.g. `nav_timegal::gal_tow()`), so no field is stored twice.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_NAV_GNSS_TIME_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_NAV_GNSS_TIME_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
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
namespace detail
{

/**
 * @brief GNSS time solution base shared by NAV-TIMEBDS/TIMEGAL/TIMEQZSS.
 *
 * @tparam Derived Named message struct (CRTP).
 * @tparam MsgId UBX-NAV message ID byte.
 */
template <typename Derived, uint8_t MsgId>
struct nav_gnss_time : fixed_message<Derived, UBX_CLASS_NAV, MsgId, 20U>
{
    /** @name `valid` bits (generic names) @{ */
    static CASTLE_CONSTEXPR uint8_t VALID_TOW = 0x01U;    ///< `tow` and `f_tow` are valid.
    static CASTLE_CONSTEXPR uint8_t VALID_WEEK = 0x02U;   ///< `week` is valid.
    static CASTLE_CONSTEXPR uint8_t VALID_LEAP_S = 0x04U; ///< `leap_s` is valid.
    /** @} */

    uint32_t i_tow = 0U;  ///< GPS time of week of the navigation epoch [ms].
    uint32_t tow = 0U;    ///< Constellation time of week, rounded [s].
    int32_t f_tow = 0;    ///< Fractional part of `tow`, +/-500000000 [ns].
    int16_t week = 0;     ///< Constellation week number.
    int8_t leap_s = 0;    ///< Constellation leap seconds (system time - UTC) [s].
    uint8_t valid = 0U;   ///< Validity flags (`VALID_*`).
    uint32_t t_acc = 0U;  ///< Time accuracy estimate [ns].

    CASTLE_NODISCARD bool tow_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_TOW) != 0U;
    }

    CASTLE_NODISCARD bool week_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_WEEK) != 0U;
    }

    CASTLE_NODISCARD bool leap_s_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_LEAP_S) != 0U;
    }

    /** @brief Precise time of week `tow + f_tow * 1e-9` [s]. */
    CASTLE_NODISCARD double time_of_week_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(tow) + (static_cast<double>(f_tow) * castle::math::pow10< -9, double>());
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.tow);
        stream.field(self.f_tow);
        stream.field(self.week);
        stream.field(self.leap_s);
        stream.field(self.valid);
        stream.field(self.t_acc);
    }
};

} // namespace detail
} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_NAV_GNSS_TIME_HPP
