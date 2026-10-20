// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_timeglo.hpp
 * @brief UBX-NAV-TIMEGLO (0x01 0x23): GLONASS time solution.
 *
 * Periodic/polled output. GLONASS reports time of day plus a day number
 * within a four-year interval, so its layout differs from the BDS/GAL/QZSS
 * family (`U2 Nt`, `U1 N4` instead of `I2 week`, `I1 leapS`).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGLO_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGLO_HPP

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

/** @brief UBX-NAV-TIMEGLO output payload. */
struct nav_timeglo : detail::fixed_message<nav_timeglo, UBX_CLASS_NAV, UBX_ID_NAV_TIMEGLO, 20U>
{
    /** @name `valid` bits @{ */
    static CASTLE_CONSTEXPR uint8_t VALID_TOD = 0x01U;   ///< `todValid`: TOD and fTOD are valid.
    static CASTLE_CONSTEXPR uint8_t VALID_DATE = 0x02U;  ///< `dateValid`: N4 and Nt are valid.
    /** @} */

    uint32_t i_tow = 0U;  ///< GPS time of week of the navigation epoch [ms].
    uint32_t tod = 0U;    ///< GLONASS time of day, rounded [s].
    int32_t f_tod = 0;    ///< Fractional part of `tod`, +/-500000000 [ns].
    uint16_t nt = 0U;     ///< Day within the four-year interval, 1..1461 [days].
    uint8_t n4 = 0U;      ///< Four-year interval number since 1996 (1 = 1996, 2 = 2000, ...).
    uint8_t valid = 0U;   ///< Validity flags (`VALID_*`).
    uint32_t t_acc = 0U;  ///< Time accuracy estimate [ns].

    CASTLE_NODISCARD bool tod_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_TOD) != 0U;
    }

    CASTLE_NODISCARD bool date_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (valid & VALID_DATE) != 0U;
    }

    /** @brief Precise GLONASS time of day `tod + f_tod * 1e-9` [s]. */
    CASTLE_NODISCARD double time_of_day_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(tod) + (static_cast<double>(f_tod) * castle::math::pow10< -9, double>());
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.tod);
        stream.field(self.f_tod);
        stream.field(self.nt);
        stream.field(self.n4);
        stream.field(self.valid);
        stream.field(self.t_acc);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_TIMEGLO_HPP
