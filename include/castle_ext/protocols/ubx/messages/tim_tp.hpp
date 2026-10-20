// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file tim_tp.hpp
 * @brief UBX-TIM-TP (0x0D 0x01): time pulse time data.
 *
 * Periodic/polled output describing the timing of the next pulse on the
 * TIMEPULSE0 output. The UTC standard enumeration is shared with
 * UBX-NAV-TIMEUTC (`nav_timeutc_utc_standard`).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_TP_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_TP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/detail/fixed_message.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timeutc.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief `timeBase` (bit 0 of `flags`). */
enum class tim_tp_time_base : uint8_t
{
    gnss = 0U, ///< Time base is GNSS.
    utc = 1U   ///< Time base is UTC.
};

/** @brief `raim` (bits 3..2 of `flags`): (T)RAIM information. */
enum class tim_tp_raim : uint8_t
{
    not_available = 0U, ///< Information not available.
    not_active = 1U,
    active = 2U
};

/** @brief `timeRefGnss` (bits 3..0 of `refInfo`); only valid when the time base is GNSS. */
enum class tim_tp_time_ref_gnss : uint8_t
{
    gps = 0U,
    glonass = 1U,
    beidou = 2U,
    galileo = 3U,
    unknown = 15U
};

/** @brief UBX-TIM-TP output payload. */
struct tim_tp : detail::fixed_message<tim_tp, UBX_CLASS_TIM, UBX_ID_TIM_TP, 16U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_TIME_BASE = 0x01U;     ///< `timeBase`: 0 GNSS, 1 UTC.
    static CASTLE_CONSTEXPR uint8_t FLAGS_UTC = 0x02U;           ///< `utc`: UTC available.
    static CASTLE_CONSTEXPR uint8_t FLAGS_RAIM = 0x0CU;          ///< `raim`.
    static CASTLE_CONSTEXPR uint8_t FLAGS_Q_ERR_INVALID = 0x10U; ///< `qErrInvalid`.
    /** @} */

    /** @name `refInfo` bits @{ */
    static CASTLE_CONSTEXPR uint8_t REF_INFO_TIME_REF_GNSS = 0x0FU; ///< `timeRefGnss`.
    static CASTLE_CONSTEXPR uint8_t REF_INFO_UTC_STANDARD = 0xF0U;  ///< `utcStandard`.
    /** @} */

    uint32_t tow_ms = 0U;     ///< Time pulse time of week according to the time base [ms].
    uint32_t tow_sub_ms = 0U; ///< Submillisecond part of `tow_ms` [2^-32 ms].
    int32_t q_err = 0;        ///< Quantization error of the time pulse [ps].
    uint16_t week = 0U;       ///< Time pulse week number according to the time base [weeks].
    uint8_t flags = 0U;       ///< Flags, see `FLAGS_*`.
    uint8_t ref_info = 0U;    ///< Time reference information, see `REF_INFO_*`.

    CASTLE_NODISCARD tim_tp_time_base time_base() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<tim_tp_time_base>(flags & FLAGS_TIME_BASE);
    }

    CASTLE_NODISCARD bool utc_available() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_UTC) != 0U;
    }

    CASTLE_NODISCARD tim_tp_raim raim() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<tim_tp_raim>((flags & FLAGS_RAIM) >> 2U);
    }

    CASTLE_NODISCARD bool q_err_invalid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_Q_ERR_INVALID) != 0U;
    }

    CASTLE_NODISCARD tim_tp_time_ref_gnss time_ref_gnss() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<tim_tp_time_ref_gnss>(ref_info & REF_INFO_TIME_REF_GNSS);
    }

    CASTLE_NODISCARD nav_timeutc_utc_standard utc_standard() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_timeutc_utc_standard>((ref_info & REF_INFO_UTC_STANDARD) >> 4U);
    }

    /** @brief `tow_sub_ms` converted to milliseconds (always in [0, 1)). */
    CASTLE_NODISCARD double tow_sub_ms_ms() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(tow_sub_ms) * TOW_SUB_MS_SCALE;
    }

    /** @brief Quantization error [ns]. */
    CASTLE_NODISCARD double q_err_ns() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(q_err) * castle::math::pow10< -3, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.tow_ms);
        stream.field(self.tow_sub_ms);
        stream.field(self.q_err);
        stream.field(self.week);
        stream.field(self.flags);
        stream.field(self.ref_info);
    }

private:
    /** @brief 2^-32, the `towSubMS` scale (exact in `double`). */
    static CASTLE_CONSTEXPR double TOW_SUB_MS_SCALE = 1.0 / 4294967296.0;
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_TP_HPP
