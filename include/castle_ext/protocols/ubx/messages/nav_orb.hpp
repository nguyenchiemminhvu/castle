// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_orb.hpp
 * @brief UBX-NAV-ORB (0x01 0x34): GNSS orbit database info.
 *
 * Periodic/polled output: an 8-byte header followed by `numSv` 6-byte
 * satellite blocks (`8 + numSv·6`).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ORB_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ORB_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/messages/detail/repeated_group.hpp"
#include "castle_ext/protocols/ubx/messages/nav_sat.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief NAV-ORB `svFlag.visibility`. */
enum class nav_orb_visibility : uint8_t
{
    unknown = 0U,
    below_horizon = 1U,
    above_horizon = 2U,
    above_elevation_mask = 3U
};

/** @brief NAV-ORB `ephSource` / `almSource`; wire values 3..7 mean "other". */
enum class nav_orb_source : uint8_t
{
    not_available = 0U,
    gnss_transmission = 1U,
    external_aiding = 2U
};

/** @brief NAV-ORB `otherOrb.type`; wire values 3..7 mean "other orbit data". */
enum class nav_orb_other_type : uint8_t
{
    none = 0U,
    assistnow_offline = 1U,
    assistnow_autonomous = 2U
};

/** @brief One satellite block of UBX-NAV-ORB. */
struct nav_orb_sv
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 6U;

    /** @name `svFlag` bits @{ */
    static CASTLE_CONSTEXPR uint8_t SV_FLAG_HEALTH = 0x03U;
    static CASTLE_CONSTEXPR uint8_t SV_FLAG_VISIBILITY = 0x0CU;
    /** @} */

    /** @name `eph` bits @{ */
    static CASTLE_CONSTEXPR uint8_t EPH_USABILITY = 0x1FU;
    static CASTLE_CONSTEXPR uint8_t EPH_SOURCE = 0xE0U;
    /** @} */

    /** @name `alm` bits @{ */
    static CASTLE_CONSTEXPR uint8_t ALM_USABILITY = 0x1FU;
    static CASTLE_CONSTEXPR uint8_t ALM_SOURCE = 0xE0U;
    /** @} */

    /** @name `otherOrb` bits @{ */
    static CASTLE_CONSTEXPR uint8_t OTHER_ORB_ANO_AOP_USABILITY = 0x1FU;
    static CASTLE_CONSTEXPR uint8_t OTHER_ORB_TYPE = 0xE0U;
    /** @} */

    /** @name Usability values shared by `ephUsability`, `almUsability` and `anoAopUsability` @{ */
    static CASTLE_CONSTEXPR uint8_t USABILITY_UNKNOWN = 31U;  ///< Usability period unknown.
    static CASTLE_CONSTEXPR uint8_t USABILITY_MAX = 30U;      ///< More than 450 min (eph) / 30 days (alm, other).
    static CASTLE_CONSTEXPR uint8_t USABILITY_EXPIRED = 0U;   ///< Data can no longer be used.
    /** @} */

    uint8_t gnss_id = 0U;   ///< GNSS identifier.
    uint8_t sv_id = 0U;     ///< Satellite identifier.
    uint8_t sv_flag = 0U;   ///< Information flags, see `SV_FLAG_*`.
    uint8_t eph = 0U;       ///< Ephemeris data, see `EPH_*`.
    uint8_t alm = 0U;       ///< Almanac data, see `ALM_*`.
    uint8_t other_orb = 0U; ///< Other orbit data, see `OTHER_ORB_*`.

    CASTLE_NODISCARD nav_sv_health health() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_sv_health>(sv_flag & SV_FLAG_HEALTH);
    }

    CASTLE_NODISCARD nav_orb_visibility visibility() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_orb_visibility>((sv_flag & SV_FLAG_VISIBILITY) >> 2U);
    }

    /** @brief 1..29: usable for (n-1)·15 to n·15 minutes; see `USABILITY_*`. */
    CASTLE_NODISCARD uint8_t eph_usability() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(eph & EPH_USABILITY);
    }

    CASTLE_NODISCARD nav_orb_source eph_source() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_orb_source>((eph & EPH_SOURCE) >> 5U);
    }

    /** @brief 1..29: usable for n-1 to n days; see `USABILITY_*`. */
    CASTLE_NODISCARD uint8_t alm_usability() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(alm & ALM_USABILITY);
    }

    CASTLE_NODISCARD nav_orb_source alm_source() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_orb_source>((alm & ALM_SOURCE) >> 5U);
    }

    /** @brief 1..29: usable for n-1 to n days; see `USABILITY_*`. */
    CASTLE_NODISCARD uint8_t ano_aop_usability() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(other_orb & OTHER_ORB_ANO_AOP_USABILITY);
    }

    CASTLE_NODISCARD nav_orb_other_type other_orb_type() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_orb_other_type>((other_orb & OTHER_ORB_TYPE) >> 5U);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.gnss_id);
        stream.field(self.sv_id);
        stream.field(self.sv_flag);
        stream.field(self.eph);
        stream.field(self.alm);
        stream.field(self.other_orb);
    }
};

/** @brief UBX-NAV-ORB output payload. */
template <castle::size_type MaxSvs = 64U>
struct nav_orb
{
    static_assert(MaxSvs <= 255U, "numSv is a U1 field");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_ORB;
    static CASTLE_CONSTEXPR castle::size_type header_length = 8U;
    static CASTLE_CONSTEXPR castle::size_type block_length = nav_orb_sv::wire_size;

    uint32_t i_tow = 0U;  ///< GPS time of week of the navigation epoch [ms].
    uint8_t version = 0U; ///< Message version (0x01 for this version).
    uint8_t num_sv = 0U;  ///< On-wire SV count; may exceed `svs.size()`.
    castle::container::vector<nav_orb_sv, MaxSvs> svs{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxSvs` satellite blocks.
     * @return `false` unless the payload length is exactly `8 + numSv·6`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_orb& out) CASTLE_NOEXCEPT
    {
        out = nav_orb{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.num_sv, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.num_sv, block_length))
        {
            out = nav_orb{};
            return false;
        }

        detail::read_group(reader, out.num_sv, out.svs);
        return reader.ok();
    }

    /** @brief Writes the header and every stored block (`numSv` is taken from `svs.size()`). */
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

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_ORB_HPP
