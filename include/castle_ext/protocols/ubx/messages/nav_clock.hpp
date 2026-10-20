// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_clock.hpp
 * @brief UBX-NAV-CLOCK (0x01 0x22): receiver clock solution.
 *
 * Periodic/polled output with the receiver clock bias, drift and their
 * accuracy estimates.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_CLOCK_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_CLOCK_HPP

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

/** @brief UBX-NAV-CLOCK output payload. */
struct nav_clock : detail::fixed_message<nav_clock, UBX_CLASS_NAV, UBX_ID_NAV_CLOCK, 20U>
{
    uint32_t i_tow = 0U;  ///< GPS time of week of the navigation epoch [ms].
    int32_t clk_b = 0;    ///< Clock bias [ns].
    int32_t clk_d = 0;    ///< Clock drift [ns/s].
    uint32_t t_acc = 0U;  ///< Time accuracy estimate [ns].
    uint32_t f_acc = 0U;  ///< Frequency accuracy estimate [ps/s].

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.clk_b);
        stream.field(self.clk_d);
        stream.field(self.t_acc);
        stream.field(self.f_acc);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_CLOCK_HPP
