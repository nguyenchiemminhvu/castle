// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_sbas.hpp
 * @brief UBX-NAV-SBAS (0x01 0x32): SBAS status data.
 *
 * Periodic/polled output: a 12-byte header followed by `cnt` 12-byte
 * SV blocks (`12 + cnt·12`).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SBAS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SBAS_HPP

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

/** @brief NAV-SBAS `mode`. */
enum class nav_sbas_mode : uint8_t
{
    disabled = 0U,
    enabled_integrity = 1U,
    enabled_test_mode = 3U
};

/** @brief NAV-SBAS `sys` / `svSys`: SBAS system. */
enum class nav_sbas_system : int8_t
{
    unknown = -1,
    waas = 0,
    egnos = 1,
    msas = 2,
    gagan = 3,
    gps = 16
};

/** @brief NAV-SBAS `statusFlags.integrityUsed`. */
enum class nav_sbas_integrity_used : uint8_t
{
    unknown = 0U,
    not_available = 1U, ///< Integrity information not available or SBAS integrity not enabled.
    gps_only = 2U       ///< Receiver uses only GPS satellites with integrity information.
};

/** @brief One SV block of UBX-NAV-SBAS. */
struct nav_sbas_sv
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 12U;

    /** @name `svService` bits (same layout as the header `service`) @{ */
    static CASTLE_CONSTEXPR uint8_t SV_SERVICE_RANGING = 0x01U;
    static CASTLE_CONSTEXPR uint8_t SV_SERVICE_CORRECTIONS = 0x02U;
    static CASTLE_CONSTEXPR uint8_t SV_SERVICE_INTEGRITY = 0x04U;
    static CASTLE_CONSTEXPR uint8_t SV_SERVICE_TESTMODE = 0x08U;
    static CASTLE_CONSTEXPR uint8_t SV_SERVICE_BAD = 0x10U;
    /** @} */

    uint8_t svid = 0U;    ///< SV ID.
    uint8_t flags = 0U;   ///< Flags for this SV (not further specified).
    uint8_t udre = 0U;    ///< Monitoring status.
    nav_sbas_system sv_sys = nav_sbas_system::unknown; ///< System, same values as `sys`.
    uint8_t sv_service = 0U; ///< Services available, see `SV_SERVICE_*`.
    int16_t prc = 0;      ///< Pseudorange correction [cm].
    int16_t ic = 0;       ///< Ionosphere correction [cm].

    CASTLE_NODISCARD bool ranging() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sv_service & SV_SERVICE_RANGING) != 0U;
    }

    CASTLE_NODISCARD bool corrections() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sv_service & SV_SERVICE_CORRECTIONS) != 0U;
    }

    CASTLE_NODISCARD bool integrity() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sv_service & SV_SERVICE_INTEGRITY) != 0U;
    }

    CASTLE_NODISCARD bool test_mode() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sv_service & SV_SERVICE_TESTMODE) != 0U;
    }

    CASTLE_NODISCARD bool bad() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sv_service & SV_SERVICE_BAD) != 0U;
    }

    /** @brief Pseudorange correction [m]. */
    CASTLE_NODISCARD double prc_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(prc) * castle::math::pow10< -2, double>();
    }

    /** @brief Ionosphere correction [m]. */
    CASTLE_NODISCARD double ic_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(ic) * castle::math::pow10< -2, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.svid);
        stream.field(self.flags);
        stream.field(self.udre);
        stream.field(self.sv_sys);
        stream.field(self.sv_service);
        stream.reserved(1U);
        stream.field(self.prc);
        stream.reserved(2U);
        stream.field(self.ic);
    }
};

/** @brief UBX-NAV-SBAS output payload. */
template <castle::size_type MaxSvs = 16U>
struct nav_sbas
{
    static_assert(MaxSvs <= 255U, "cnt is a U1 field");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_SBAS;
    static CASTLE_CONSTEXPR castle::size_type header_length = 12U;
    static CASTLE_CONSTEXPR castle::size_type block_length = nav_sbas_sv::wire_size;

    /** @name `service` bits @{ */
    static CASTLE_CONSTEXPR uint8_t SERVICE_RANGING = nav_sbas_sv::SV_SERVICE_RANGING;
    static CASTLE_CONSTEXPR uint8_t SERVICE_CORRECTIONS = nav_sbas_sv::SV_SERVICE_CORRECTIONS;
    static CASTLE_CONSTEXPR uint8_t SERVICE_INTEGRITY = nav_sbas_sv::SV_SERVICE_INTEGRITY;
    static CASTLE_CONSTEXPR uint8_t SERVICE_TESTMODE = nav_sbas_sv::SV_SERVICE_TESTMODE;
    static CASTLE_CONSTEXPR uint8_t SERVICE_BAD = nav_sbas_sv::SV_SERVICE_BAD;
    /** @} */

    /** @name `statusFlags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t STATUS_FLAGS_INTEGRITY_USED = 0x03U;
    /** @} */

    uint32_t i_tow = 0U; ///< GPS time of week of the navigation epoch [ms].
    uint8_t geo = 0U;    ///< PRN of the GEO providing correction and integrity data.
    nav_sbas_mode mode = nav_sbas_mode::disabled;
    nav_sbas_system sys = nav_sbas_system::unknown;
    uint8_t service = 0U;      ///< SBAS services available, see `SERVICE_*`.
    uint8_t cnt = 0U;          ///< On-wire SV count; may exceed `svs.size()`.
    uint8_t status_flags = 0U; ///< SBAS status flags, see `STATUS_FLAGS_*`.
    castle::container::vector<nav_sbas_sv, MaxSvs> svs{};

    CASTLE_NODISCARD bool ranging() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (service & SERVICE_RANGING) != 0U;
    }

    CASTLE_NODISCARD bool corrections() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (service & SERVICE_CORRECTIONS) != 0U;
    }

    CASTLE_NODISCARD bool integrity() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (service & SERVICE_INTEGRITY) != 0U;
    }

    CASTLE_NODISCARD bool test_mode() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (service & SERVICE_TESTMODE) != 0U;
    }

    CASTLE_NODISCARD bool bad() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (service & SERVICE_BAD) != 0U;
    }

    CASTLE_NODISCARD nav_sbas_integrity_used integrity_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_sbas_integrity_used>(status_flags & STATUS_FLAGS_INTEGRITY_USED);
    }

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxSvs` SV blocks.
     * @return `false` unless the payload length is exactly `12 + cnt·12`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_sbas& out) CASTLE_NOEXCEPT
    {
        out = nav_sbas{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.cnt, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.cnt, block_length))
        {
            out = nav_sbas{};
            return false;
        }

        detail::read_group(reader, out.cnt, out.svs);
        return reader.ok();
    }

    /** @brief Writes the header and every stored block (`cnt` is taken from `svs.size()`). */
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
        stream.field(self.geo);
        stream.field(self.mode);
        stream.field(self.sys);
        stream.field(self.service);
        stream.field(count);
        stream.field(self.status_flags);
        stream.reserved(2U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SBAS_HPP
