// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_att.hpp
 * @brief UBX-NAV-ATT (0x01 0x05): vehicle attitude solution.
 *
 * Periodic/polled output with roll, pitch and heading angles. An accuracy of
 * zero means the corresponding angle is not available.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ATT_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ATT_HPP

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

/** @brief UBX-NAV-ATT output payload. */
struct nav_att : detail::fixed_message<nav_att, UBX_CLASS_NAV, UBX_ID_NAV_ATT, 32U>
{
    uint32_t i_tow = 0U;        ///< GPS time of week [ms].
    uint8_t version = 0U;       ///< Message version (0x00 for this version).
    int32_t roll = 0;           ///< Vehicle roll [1e-5 deg].
    int32_t pitch = 0;          ///< Vehicle pitch [1e-5 deg].
    int32_t heading = 0;        ///< Vehicle heading [1e-5 deg].
    uint32_t acc_roll = 0U;     ///< Roll accuracy, 0 = roll not available [1e-5 deg].
    uint32_t acc_pitch = 0U;    ///< Pitch accuracy, 0 = pitch not available [1e-5 deg].
    uint32_t acc_heading = 0U;  ///< Heading accuracy, 0 = heading not available [1e-5 deg].

    CASTLE_NODISCARD double roll_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(roll) * castle::math::pow10< -5, double>();
    }

    CASTLE_NODISCARD double pitch_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(pitch) * castle::math::pow10< -5, double>();
    }

    CASTLE_NODISCARD double heading_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(heading) * castle::math::pow10< -5, double>();
    }

    CASTLE_NODISCARD double acc_roll_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(acc_roll) * castle::math::pow10< -5, double>();
    }

    CASTLE_NODISCARD double acc_pitch_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(acc_pitch) * castle::math::pow10< -5, double>();
    }

    CASTLE_NODISCARD double acc_heading_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(acc_heading) * castle::math::pow10< -5, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.version);
        stream.reserved(3U);
        stream.field(self.roll);
        stream.field(self.pitch);
        stream.field(self.heading);
        stream.field(self.acc_roll);
        stream.field(self.acc_pitch);
        stream.field(self.acc_heading);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ATT_HPP
