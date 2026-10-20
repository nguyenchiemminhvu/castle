// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file tim_vrfy.hpp
 * @brief UBX-TIM-VRFY (0x0D 0x06): sourced time verification.
 *
 * Periodic/polled output comparing the time previously received from
 * assistance data or the RTC with the current receiver time.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_VRFY_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_VRFY_HPP

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

/** @brief `src` (bits 2..0 of `flags`): aiding time source. */
enum class tim_vrfy_src : uint8_t
{
    no_aiding = 0U,     ///< No time aiding done.
    rtc = 2U,           ///< Source was the RTC.
    assistance = 3U     ///< Source was assistance data.
};

/** @brief UBX-TIM-VRFY output payload. */
struct tim_vrfy : detail::fixed_message<tim_vrfy, UBX_CLASS_TIM, UBX_ID_TIM_VRFY, 20U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_SRC = 0x07U; ///< `src`.
    /** @} */

    int32_t itow = 0;      ///< Integer millisecond time of week received by the source [ms].
    int32_t frac = 0;      ///< Sub-millisecond part of the time of week [ns].
    int32_t delta_ms = 0;  ///< Integer milliseconds of current time minus sourced time [ms].
    int32_t delta_ns = 0;  ///< Sub-millisecond part of the delta time [ns].
    uint16_t wno = 0U;     ///< Week number [weeks].
    uint8_t flags = 0U;    ///< Flags, see `FLAGS_*`.

    CASTLE_NODISCARD tim_vrfy_src src() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<tim_vrfy_src>(flags & FLAGS_SRC);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.itow);
        stream.field(self.frac);
        stream.field(self.delta_ms);
        stream.field(self.delta_ns);
        stream.field(self.wno);
        stream.field(self.flags);
        stream.reserved(1U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_TIM_VRFY_HPP
