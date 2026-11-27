// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_relposned.hpp
 * @brief UBX-NAV-RELPOSNED (0x01 0x3C): relative position from the reference station in NED.
 *
 * Periodic/polled output (version 0x01 layout). Each vector component is
 * split into a centimetre part (`rel_pos_n`) and a 0.1 mm high-precision part
 * (`rel_pos_hp_n`, -99..+99): full component [cm] = relPosN + relPosHPN * 1e-2.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_RELPOSNED_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_RELPOSNED_HPP

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

/** @brief Carrier phase range solution status (`flags.carrSoln`). */
enum class nav_relposned_carr_soln : uint8_t
{
    none = 0U,     ///< No carrier phase range solution.
    floating = 1U, ///< Carrier phase range solution with floating ambiguities.
    fixed = 2U     ///< Carrier phase range solution with fixed ambiguities.
};

/** @brief UBX-NAV-RELPOSNED output payload. */
struct nav_relposned : detail::fixed_message<nav_relposned, UBX_CLASS_NAV, UBX_ID_NAV_RELPOSNED, 64U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint32_t FLAGS_GNSS_FIX_OK = 0x00000001U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_DIFF_SOLN = 0x00000002U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_REL_POS_VALID = 0x00000004U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_CARR_SOLN_MASK = 0x00000018U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_CARR_SOLN_SHIFT = 3U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_IS_MOVING = 0x00000020U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_REF_POS_MISS = 0x00000040U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_REF_OBS_MISS = 0x00000080U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_REL_POS_HEADING_VALID = 0x00000100U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_REL_POS_NORMALIZED = 0x00000200U;
    /** @} */

    uint8_t version = 0U;            ///< Message version (0x01 for this layout).
    uint16_t ref_station_id = 0U;    ///< Reference station ID, 0..4095.
    uint32_t i_tow = 0U;             ///< GPS time of week [ms].
    int32_t rel_pos_n = 0;           ///< North component of relative position [cm].
    int32_t rel_pos_e = 0;           ///< East component of relative position [cm].
    int32_t rel_pos_d = 0;           ///< Down component of relative position [cm].
    int32_t rel_pos_length = 0;      ///< Length of the relative position vector [cm].
    int32_t rel_pos_heading = 0;     ///< Heading of the relative position vector [1e-5 deg].
    int8_t rel_pos_hp_n = 0;         ///< High-precision North component [0.1 mm], -99..+99.
    int8_t rel_pos_hp_e = 0;         ///< High-precision East component [0.1 mm], -99..+99.
    int8_t rel_pos_hp_d = 0;         ///< High-precision Down component [0.1 mm], -99..+99.
    int8_t rel_pos_hp_length = 0;    ///< High-precision length component [0.1 mm], -99..+99.
    uint32_t acc_n = 0U;             ///< North component accuracy [0.1 mm].
    uint32_t acc_e = 0U;             ///< East component accuracy [0.1 mm].
    uint32_t acc_d = 0U;             ///< Down component accuracy [0.1 mm].
    uint32_t acc_length = 0U;        ///< Length accuracy [0.1 mm].
    uint32_t acc_heading = 0U;       ///< Heading accuracy [1e-5 deg].
    uint32_t flags = 0U;             ///< Solution flags, see `FLAGS_*`.

    CASTLE_NODISCARD bool gnss_fix_ok() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_GNSS_FIX_OK) != 0U;
    }

    CASTLE_NODISCARD bool diff_soln() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_DIFF_SOLN) != 0U;
    }

    CASTLE_NODISCARD bool rel_pos_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_REL_POS_VALID) != 0U;
    }

    /** @brief Returns `flags.carrSoln` (bits 4..3); the reserved value 3 is passed through unchanged. */
    CASTLE_NODISCARD nav_relposned_carr_soln carr_soln() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_relposned_carr_soln>((flags & FLAGS_CARR_SOLN_MASK) >> FLAGS_CARR_SOLN_SHIFT);
    }

    CASTLE_NODISCARD bool is_moving() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_IS_MOVING) != 0U;
    }

    CASTLE_NODISCARD bool ref_pos_miss() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_REF_POS_MISS) != 0U;
    }

    CASTLE_NODISCARD bool ref_obs_miss() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_REF_OBS_MISS) != 0U;
    }

    CASTLE_NODISCARD bool rel_pos_heading_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_REL_POS_HEADING_VALID) != 0U;
    }

    CASTLE_NODISCARD bool rel_pos_normalized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_REL_POS_NORMALIZED) != 0U;
    }

    /** @brief Full North component [m] = relPosN [cm] + relPosHPN [0.1 mm]. */
    CASTLE_NODISCARD double rel_pos_n_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_m(rel_pos_n, rel_pos_hp_n);
    }

    /** @brief Full East component [m] = relPosE [cm] + relPosHPE [0.1 mm]. */
    CASTLE_NODISCARD double rel_pos_e_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_m(rel_pos_e, rel_pos_hp_e);
    }

    /** @brief Full Down component [m] = relPosD [cm] + relPosHPD [0.1 mm]. */
    CASTLE_NODISCARD double rel_pos_d_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_m(rel_pos_d, rel_pos_hp_d);
    }

    /** @brief Full vector length [m] = relPosLength [cm] + relPosHPLength [0.1 mm]. */
    CASTLE_NODISCARD double rel_pos_length_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return precise_m(rel_pos_length, rel_pos_hp_length);
    }

    CASTLE_NODISCARD double rel_pos_heading_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(rel_pos_heading) * castle::math::pow10< -5, double>();
    }

    CASTLE_NODISCARD double acc_n_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(acc_n) * castle::math::pow10< -4, double>();
    }

    CASTLE_NODISCARD double acc_e_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(acc_e) * castle::math::pow10< -4, double>();
    }

    CASTLE_NODISCARD double acc_d_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(acc_d) * castle::math::pow10< -4, double>();
    }

    CASTLE_NODISCARD double acc_length_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(acc_length) * castle::math::pow10< -4, double>();
    }

    CASTLE_NODISCARD double acc_heading_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(acc_heading) * castle::math::pow10< -5, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.reserved(1U);
        stream.field(self.ref_station_id);
        stream.field(self.i_tow);
        stream.field(self.rel_pos_n);
        stream.field(self.rel_pos_e);
        stream.field(self.rel_pos_d);
        stream.field(self.rel_pos_length);
        stream.field(self.rel_pos_heading);
        stream.reserved(4U);
        stream.field(self.rel_pos_hp_n);
        stream.field(self.rel_pos_hp_e);
        stream.field(self.rel_pos_hp_d);
        stream.field(self.rel_pos_hp_length);
        stream.field(self.acc_n);
        stream.field(self.acc_e);
        stream.field(self.acc_d);
        stream.field(self.acc_length);
        stream.field(self.acc_heading);
        stream.reserved(4U);
        stream.field(self.flags);
    }

private:
    static double precise_m(int32_t cm, int8_t hp_0_1mm) CASTLE_NOEXCEPT
    {
        return (static_cast<double>(cm) * castle::math::pow10< -2, double>())
            + (static_cast<double>(hp_0_1mm) * castle::math::pow10< -4, double>());
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_RELPOSNED_HPP
