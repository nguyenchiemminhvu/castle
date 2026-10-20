// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_slas.hpp
 * @brief UBX-NAV-SLAS (0x01 0x42): QZSS L1S SLAS status data.
 *
 * Periodic/polled output: a 20-byte header followed by `cnt` 8-byte
 * pseudorange-correction blocks (`20 + cnt·8`).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SLAS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SLAS_HPP

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

/** @brief One pseudorange-correction block of UBX-NAV-SLAS. */
struct nav_slas_correction
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 8U;

    uint8_t gnss_id = 0U; ///< GNSS identifier (see Satellite Numbering).
    uint8_t sv_id = 0U;   ///< Satellite identifier (see Satellite Numbering).
    int16_t prc = 0;      ///< Pseudorange correction [cm].

    /** @brief Pseudorange correction [m]. */
    CASTLE_NODISCARD double prc_m() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(prc) * castle::math::pow10< -2, double>();
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.gnss_id);
        stream.field(self.sv_id);
        stream.reserved(1U); // reserved1
        stream.reserved(3U); // reserved2
        stream.field(self.prc);
    }
};

/** @brief UBX-NAV-SLAS output payload. */
template <castle::size_type MaxCorrections = 16U>
struct nav_slas
{
    static_assert(MaxCorrections <= 255U, "cnt is a U1 field");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_SLAS;
    static CASTLE_CONSTEXPR castle::size_type header_length = 20U;
    static CASTLE_CONSTEXPR castle::size_type block_length = nav_slas_correction::wire_size;

    /** @name `serviceFlags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t SERVICE_FLAGS_GMS_AVAILABLE = 0x01U;
    static CASTLE_CONSTEXPR uint8_t SERVICE_FLAGS_QZSS_SV_AVAILABLE = 0x02U;
    static CASTLE_CONSTEXPR uint8_t SERVICE_FLAGS_TEST_MODE = 0x04U;
    /** @} */

    uint32_t i_tow = 0U;        ///< GPS time of week of the navigation epoch [ms].
    uint8_t version = 0U;       ///< Message version (0x00 for this version).
    int32_t gms_lon = 0;        ///< Longitude of the used ground monitoring station [1e-3 deg].
    int32_t gms_lat = 0;        ///< Latitude of the used ground monitoring station [1e-3 deg].
    uint8_t gms_code = 0U;      ///< Ground monitoring station code (QZSS SLAS IS).
    uint8_t qzss_sv_id = 0U;    ///< QZS/GEO whose correction data is used.
    uint8_t service_flags = 0U; ///< SLAS service flags, see `SERVICE_FLAGS_*`.
    uint8_t cnt = 0U;           ///< On-wire correction count; may exceed `corrections.size()`.
    castle::container::vector<nav_slas_correction, MaxCorrections> corrections{};

    CASTLE_NODISCARD bool gms_available() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (service_flags & SERVICE_FLAGS_GMS_AVAILABLE) != 0U;
    }

    CASTLE_NODISCARD bool qzss_sv_available() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (service_flags & SERVICE_FLAGS_QZSS_SV_AVAILABLE) != 0U;
    }

    CASTLE_NODISCARD bool test_mode() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (service_flags & SERVICE_FLAGS_TEST_MODE) != 0U;
    }

    /** @brief Ground monitoring station longitude [deg]. */
    CASTLE_NODISCARD double gms_longitude_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(gms_lon) * castle::math::pow10< -3, double>();
    }

    /** @brief Ground monitoring station latitude [deg]. */
    CASTLE_NODISCARD double gms_latitude_deg() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(gms_lat) * castle::math::pow10< -3, double>();
    }

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxCorrections` correction blocks.
     * @return `false` unless the payload length is exactly `20 + cnt·8`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_slas& out) CASTLE_NOEXCEPT
    {
        out = nav_slas{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.cnt, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.cnt, block_length))
        {
            out = nav_slas{};
            return false;
        }

        detail::read_group(reader, out.cnt, out.corrections);
        return reader.ok();
    }

    /** @brief Writes the header and every stored block (`cnt` is taken from `corrections.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t count = static_cast<uint8_t>(corrections.size());
        layout_header(*this, count, writer);
        detail::write_group(writer, corrections);
    }

private:
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& count, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.version);
        stream.reserved(3U);
        stream.field(self.gms_lon);
        stream.field(self.gms_lat);
        stream.field(self.gms_code);
        stream.field(self.qzss_sv_id);
        stream.field(self.service_flags);
        stream.field(count);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SLAS_HPP
