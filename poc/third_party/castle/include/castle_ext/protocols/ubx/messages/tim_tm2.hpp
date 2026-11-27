// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file tim_tm2.hpp
 * @brief UBX-TIM-TM2 (0x0D 0x03): time mark data.
 *
 * Periodic/polled output for high-precision time stamping / pulse counting on
 * an EXTINT channel. The CFG-TP delays and time base also apply here.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_TM2_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_TM2_HPP

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

/** @brief `timeBase` (bits 4..3 of `flags`): time base of the reported time marks. */
enum class tim_tm2_time_base : uint8_t
{
    receiver = 0U, ///< Receiver time.
    gnss = 1U,     ///< GNSS time (system configured in CFG-TP for tpIdx=0).
    utc = 2U       ///< UTC (variant configured in CFG-NAVSPG-*).
};

/** @brief UBX-TIM-TM2 output payload. */
struct tim_tm2 : detail::fixed_message<tim_tm2, UBX_CLASS_TIM, UBX_ID_TIM_TM2, 28U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_MODE = 0x01U;             ///< `mode`: 0 single, 1 running.
    static CASTLE_CONSTEXPR uint8_t FLAGS_RUN = 0x02U;              ///< `run`: 0 armed, 1 stopped.
    static CASTLE_CONSTEXPR uint8_t FLAGS_NEW_FALLING_EDGE = 0x04U; ///< `newFallingEdge`.
    static CASTLE_CONSTEXPR uint8_t FLAGS_TIME_BASE = 0x18U;        ///< `timeBase`.
    static CASTLE_CONSTEXPR uint8_t FLAGS_UTC = 0x20U;              ///< `utc`: UTC available.
    static CASTLE_CONSTEXPR uint8_t FLAGS_TIME = 0x40U;             ///< `time`: time valid (valid GNSS fix).
    static CASTLE_CONSTEXPR uint8_t FLAGS_NEW_RISING_EDGE = 0x80U;  ///< `newRisingEdge`.
    /** @} */

    uint8_t ch = 0U;              ///< Channel (EXTINT) on which the pulse was measured.
    uint8_t flags = 0U;           ///< Bitmask, see `FLAGS_*`.
    uint16_t count = 0U;          ///< Rising edge counter.
    uint16_t wn_r = 0U;           ///< Week number of the last rising edge [weeks].
    uint16_t wn_f = 0U;           ///< Week number of the last falling edge [weeks].
    uint32_t tow_ms_r = 0U;       ///< Time of week of the rising edge [ms].
    uint32_t tow_sub_ms_r = 0U;   ///< Millisecond fraction of the rising-edge time of week [ns].
    uint32_t tow_ms_f = 0U;       ///< Time of week of the falling edge [ms].
    uint32_t tow_sub_ms_f = 0U;   ///< Millisecond fraction of the falling-edge time of week [ns].
    uint32_t acc_est = 0U;        ///< Accuracy estimate [ns].

    /** @brief `mode` bit: `true` = running, `false` = single. */
    CASTLE_NODISCARD bool running() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_MODE) != 0U;
    }

    /** @brief `run` bit: `true` = stopped, `false` = armed. */
    CASTLE_NODISCARD bool stopped() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_RUN) != 0U;
    }

    CASTLE_NODISCARD bool new_falling_edge() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_NEW_FALLING_EDGE) != 0U;
    }

    CASTLE_NODISCARD tim_tm2_time_base time_base() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<tim_tm2_time_base>((flags & FLAGS_TIME_BASE) >> 3U);
    }

    CASTLE_NODISCARD bool utc_available() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_UTC) != 0U;
    }

    CASTLE_NODISCARD bool time_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_TIME) != 0U;
    }

    CASTLE_NODISCARD bool new_rising_edge() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_NEW_RISING_EDGE) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.ch);
        stream.field(self.flags);
        stream.field(self.count);
        stream.field(self.wn_r);
        stream.field(self.wn_f);
        stream.field(self.tow_ms_r);
        stream.field(self.tow_sub_ms_r);
        stream.field(self.tow_ms_f);
        stream.field(self.tow_sub_ms_f);
        stream.field(self.acc_est);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_TM2_HPP
