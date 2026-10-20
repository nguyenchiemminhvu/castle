// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_sat.hpp
 * @brief UBX-NAV-SAT (0x01 0x35): satellite information.
 *
 * Periodic/polled output: an 8-byte header followed by `numSvs` 12-byte
 * satellite blocks (`8 + numSvs·12`). Also defines the health and signal
 * quality enumerations shared with UBX-NAV-SIG and UBX-NAV-ORB.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SAT_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SAT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/messages/detail/repeated_group.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief SV / signal health (NAV-SAT `health`, NAV-SIG `health`, NAV-ORB `health`). */
enum class nav_sv_health : uint8_t
{
    unknown = 0U,
    healthy = 1U,
    unhealthy = 2U
};

/**
 * @brief Signal quality indicator (NAV-SAT `qualityInd`, NAV-SIG `qualityInd`).
 * @note Wire values 6 and 7 mean the same as `code_carrier_locked` (5).
 */
enum class nav_signal_quality : uint8_t
{
    no_signal = 0U,
    searching = 1U,
    acquired = 2U,
    unusable = 3U,            ///< Signal detected but unusable.
    code_locked = 4U,         ///< Code locked and time synchronized.
    code_carrier_locked = 5U  ///< Code and carrier locked and time synchronized.
};

/** @brief NAV-SAT `orbitSource`; wire values 5..7 mean "other orbit information". */
enum class nav_sat_orbit_source : uint8_t
{
    none = 0U,
    ephemeris = 1U,
    almanac = 2U,
    assistnow_offline = 3U,
    assistnow_autonomous = 4U
};

/** @brief One satellite block of UBX-NAV-SAT. */
struct nav_sat_sv
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 12U;

    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint32_t FLAGS_QUALITY_IND = 0x00000007U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_SV_USED = 0x00000008U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_HEALTH = 0x00000030U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_DIFF_CORR = 0x00000040U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_SMOOTHED = 0x00000080U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_ORBIT_SOURCE = 0x00000700U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_EPH_AVAIL = 0x00000800U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_ALM_AVAIL = 0x00001000U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_ANO_AVAIL = 0x00002000U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_AOP_AVAIL = 0x00004000U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_SBAS_CORR_USED = 0x00010000U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_RTCM_CORR_USED = 0x00020000U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_SLAS_CORR_USED = 0x00040000U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_SPARTN_CORR_USED = 0x00080000U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_PR_CORR_USED = 0x00100000U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_CR_CORR_USED = 0x00200000U;
    static CASTLE_CONSTEXPR uint32_t FLAGS_DO_CORR_USED = 0x00400000U;
    /** @} */

    uint8_t gnss_id = 0U; ///< GNSS identifier (see Satellite Numbering).
    uint8_t sv_id = 0U;   ///< Satellite identifier (see Satellite Numbering).
    uint8_t cno = 0U;     ///< Carrier-to-noise ratio [dBHz].
    int8_t elev = 0;      ///< Elevation [deg], range +/-90; unknown if out of range.
    int16_t azim = 0;     ///< Azimuth [deg], range 0..360; unknown if elevation is out of range.
    int16_t pr_res = 0;   ///< Pseudorange residual [0.1 m].
    uint32_t flags = 0U;  ///< Bitmask, see `FLAGS_*`.

    CASTLE_NODISCARD nav_signal_quality quality_ind() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_signal_quality>(flags & FLAGS_QUALITY_IND);
    }

    CASTLE_NODISCARD bool sv_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_SV_USED) != 0U;
    }

    CASTLE_NODISCARD nav_sv_health health() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_sv_health>((flags & FLAGS_HEALTH) >> 4U);
    }

    CASTLE_NODISCARD bool diff_corr() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_DIFF_CORR) != 0U;
    }

    CASTLE_NODISCARD bool smoothed() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_SMOOTHED) != 0U;
    }

    CASTLE_NODISCARD nav_sat_orbit_source orbit_source() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_sat_orbit_source>((flags & FLAGS_ORBIT_SOURCE) >> 8U);
    }

    CASTLE_NODISCARD bool eph_avail() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_EPH_AVAIL) != 0U;
    }

    CASTLE_NODISCARD bool alm_avail() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_ALM_AVAIL) != 0U;
    }

    CASTLE_NODISCARD bool ano_avail() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_ANO_AVAIL) != 0U;
    }

    CASTLE_NODISCARD bool aop_avail() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_AOP_AVAIL) != 0U;
    }

    CASTLE_NODISCARD bool sbas_corr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_SBAS_CORR_USED) != 0U;
    }

    CASTLE_NODISCARD bool rtcm_corr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_RTCM_CORR_USED) != 0U;
    }

    CASTLE_NODISCARD bool slas_corr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_SLAS_CORR_USED) != 0U;
    }

    CASTLE_NODISCARD bool spartn_corr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_SPARTN_CORR_USED) != 0U;
    }

    CASTLE_NODISCARD bool pr_corr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_PR_CORR_USED) != 0U;
    }

    CASTLE_NODISCARD bool cr_corr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_CR_CORR_USED) != 0U;
    }

    CASTLE_NODISCARD bool do_corr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_DO_CORR_USED) != 0U;
    }

    /** @brief Pseudorange residual [m]. */
    CASTLE_NODISCARD double pr_res_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(pr_res) * castle::math::pow10< -1, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.gnss_id);
        stream.field(self.sv_id);
        stream.field(self.cno);
        stream.field(self.elev);
        stream.field(self.azim);
        stream.field(self.pr_res);
        stream.field(self.flags);
    }
};

/** @brief UBX-NAV-SAT output payload. */
template <castle::size_type MaxSvs = 64U>
struct nav_sat
{
    static_assert(MaxSvs <= 255U, "numSvs is a U1 field");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_SAT;
    static CASTLE_CONSTEXPR castle::size_type header_length = 8U;
    static CASTLE_CONSTEXPR castle::size_type block_length = nav_sat_sv::wire_size;

    uint32_t i_tow = 0U;  ///< GPS time of week of the navigation epoch [ms].
    uint8_t version = 0U; ///< Message version (0x01 for this version).
    uint8_t num_svs = 0U; ///< On-wire satellite count; may exceed `svs.size()`.
    castle::container::vector<nav_sat_sv, MaxSvs> svs{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxSvs` satellite blocks.
     * @return `false` unless the payload length is exactly `8 + numSvs·12`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_sat& out) CASTLE_NOEXCEPT
    {
        out = nav_sat{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.num_svs, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.num_svs, block_length))
        {
            out = nav_sat{};
            return false;
        }

        detail::read_group(reader, out.num_svs, out.svs);
        return reader.ok();
    }

    /** @brief Writes the header and every stored block (`numSvs` is taken from `svs.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t count = static_cast<uint8_t>(svs.size());
        layout_header(*this, count, writer);
        detail::write_group(writer, svs);
    }

private:
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& count, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.version);
        stream.field(count);
        stream.reserved(2U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SAT_HPP
