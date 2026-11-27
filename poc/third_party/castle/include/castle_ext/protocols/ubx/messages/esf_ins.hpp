// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file esf_ins.hpp
 * @brief UBX-ESF-INS (0x10 0x15): vehicle dynamics information.
 *
 * Periodic/polled output of the compensated angular rates and gravity-free
 * accelerations, expressed in the vehicle frame.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_INS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_INS_HPP

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

/** @brief UBX-ESF-INS output payload. */
struct esf_ins : detail::fixed_message<esf_ins, UBX_CLASS_ESF, UBX_ID_ESF_INS, 36U>
{
    /** @name `bitfield0` bits @{ */
    static CASTLE_CONSTEXPR uint32_t BF0_VERSION = 0x000000FFU;
    static CASTLE_CONSTEXPR uint32_t BF0_X_ANG_RATE_VALID = 0x00000100U;
    static CASTLE_CONSTEXPR uint32_t BF0_Y_ANG_RATE_VALID = 0x00000200U;
    static CASTLE_CONSTEXPR uint32_t BF0_Z_ANG_RATE_VALID = 0x00000400U;
    static CASTLE_CONSTEXPR uint32_t BF0_X_ACCEL_VALID = 0x00000800U;
    static CASTLE_CONSTEXPR uint32_t BF0_Y_ACCEL_VALID = 0x00001000U;
    static CASTLE_CONSTEXPR uint32_t BF0_Z_ACCEL_VALID = 0x00002000U;
    /** @} */

    uint32_t bitfield0 = 0U; ///< Message version (bits 7..0) and `BF0_*_VALID` bits.
    uint32_t i_tow = 0U;     ///< GPS time of week of the navigation epoch [ms].
    int32_t x_ang_rate = 0;  ///< Compensated x-axis angular rate [1e-3 deg/s].
    int32_t y_ang_rate = 0;  ///< Compensated y-axis angular rate [1e-3 deg/s].
    int32_t z_ang_rate = 0;  ///< Compensated z-axis angular rate [1e-3 deg/s].
    int32_t x_accel = 0;     ///< Compensated x-axis acceleration (gravity-free) [1e-2 m/s^2].
    int32_t y_accel = 0;     ///< Compensated y-axis acceleration (gravity-free) [1e-2 m/s^2].
    int32_t z_accel = 0;     ///< Compensated z-axis acceleration (gravity-free) [1e-2 m/s^2].

    /** @brief Returns the message version (`bitfield0` bits 7..0; 0x01 for this version). */
    CASTLE_NODISCARD uint8_t version() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(bitfield0 & BF0_VERSION);
    }

    CASTLE_NODISCARD bool x_ang_rate_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (bitfield0 & BF0_X_ANG_RATE_VALID) != 0U;
    }

    CASTLE_NODISCARD bool y_ang_rate_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (bitfield0 & BF0_Y_ANG_RATE_VALID) != 0U;
    }

    CASTLE_NODISCARD bool z_ang_rate_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (bitfield0 & BF0_Z_ANG_RATE_VALID) != 0U;
    }

    CASTLE_NODISCARD bool x_accel_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (bitfield0 & BF0_X_ACCEL_VALID) != 0U;
    }

    CASTLE_NODISCARD bool y_accel_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (bitfield0 & BF0_Y_ACCEL_VALID) != 0U;
    }

    CASTLE_NODISCARD bool z_accel_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (bitfield0 & BF0_Z_ACCEL_VALID) != 0U;
    }

    CASTLE_NODISCARD double x_ang_rate_deg_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(x_ang_rate) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double y_ang_rate_deg_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(y_ang_rate) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double z_ang_rate_deg_s() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(z_ang_rate) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD double x_accel_m_s2() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(x_accel) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double y_accel_m_s2() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(y_accel) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double z_accel_m_s2() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(z_accel) * castle::math::pow10< -2, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.bitfield0);
        stream.reserved(4U);
        stream.field(self.i_tow);
        stream.field(self.x_ang_rate);
        stream.field(self.y_ang_rate);
        stream.field(self.z_ang_rate);
        stream.field(self.x_accel);
        stream.field(self.y_accel);
        stream.field(self.z_accel);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_INS_HPP
