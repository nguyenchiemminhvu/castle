// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file esf_alg.hpp
 * @brief UBX-ESF-ALG (0x10 0x14): IMU-mount alignment information.
 *
 * Periodic/polled output carrying the IMU-mount alignment angles (rotation
 * from the installation frame to the IMU frame) and the automatic alignment
 * status and error flags.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_ALG_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_ALG_HPP

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

/** @brief IMU-mount alignment status (`flags` bits 3..1). */
enum class esf_alg_status : uint8_t
{
    user_defined = 0U,           ///< User-defined/fixed angles are used.
    roll_pitch_ongoing = 1U,     ///< IMU-mount roll/pitch angles alignment is ongoing.
    roll_pitch_yaw_ongoing = 2U, ///< IMU-mount roll/pitch/yaw angles alignment is ongoing.
    coarse = 3U,                 ///< Coarse IMU-mount alignment angles are used.
    fine = 4U                    ///< Fine IMU-mount alignment angles are used.
};

/** @brief UBX-ESF-ALG output payload. */
struct esf_alg : detail::fixed_message<esf_alg, UBX_CLASS_ESF, UBX_ID_ESF_ALG, 16U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_AUTO_MNT_ALG_ON = 0x01U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_STATUS = 0x0EU;
    /** @} */

    /** @name `error` bits @{ */
    static CASTLE_CONSTEXPR uint8_t ERROR_TILT_ALG_ERROR = 0x01U;
    static CASTLE_CONSTEXPR uint8_t ERROR_YAW_ALG_ERROR = 0x02U;
    static CASTLE_CONSTEXPR uint8_t ERROR_ANGLE_ERROR = 0x04U;
    /** @} */

    uint32_t i_tow = 0U;  ///< GPS time of week of the navigation epoch [ms].
    uint8_t version = 0U; ///< Message version (0x01 for this version).
    uint8_t flags = 0U;   ///< `FLAGS_*` bits.
    uint8_t error = 0U;   ///< `ERROR_*` bits.
    uint32_t yaw = 0U;    ///< IMU-mount yaw angle [1e-2 deg], range [0, 360].
    int16_t pitch = 0;    ///< IMU-mount pitch angle [1e-2 deg], range [-90, 90].
    int16_t roll = 0;     ///< IMU-mount roll angle [1e-2 deg], range [-180, 180].

    /** @brief Reports whether automatic IMU-mount alignment is running. */
    CASTLE_NODISCARD bool auto_mnt_alg_on() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_AUTO_MNT_ALG_ON) != 0U;
    }

    /** @brief Returns the IMU-mount alignment status (`flags` bits 3..1). */
    CASTLE_NODISCARD esf_alg_status status() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<esf_alg_status>((flags & FLAGS_STATUS) >> 1U);
    }

    /** @brief Reports an IMU-mount tilt (roll and/or pitch) alignment error. */
    CASTLE_NODISCARD bool tilt_alg_error() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (error & ERROR_TILT_ALG_ERROR) != 0U;
    }

    /** @brief Reports an IMU-mount yaw alignment error. */
    CASTLE_NODISCARD bool yaw_alg_error() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (error & ERROR_YAW_ALG_ERROR) != 0U;
    }

    /** @brief Reports the Euler-angle singularity ("gimbal lock") error. */
    CASTLE_NODISCARD bool angle_error() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (error & ERROR_ANGLE_ERROR) != 0U;
    }

    CASTLE_NODISCARD double yaw_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(yaw) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double pitch_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(pitch) * castle::math::pow10< -2, double>();
    }

    CASTLE_NODISCARD double roll_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(roll) * castle::math::pow10< -2, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.version);
        stream.field(self.flags);
        stream.field(self.error);
        stream.reserved(1U);
        stream.field(self.yaw);
        stream.field(self.pitch);
        stream.field(self.roll);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_ALG_HPP
