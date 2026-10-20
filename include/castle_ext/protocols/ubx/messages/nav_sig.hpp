// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_sig.hpp
 * @brief UBX-NAV-SIG (0x01 0x43): signal information.
 *
 * Periodic/polled output: an 8-byte header followed by `numSigs` 16-byte
 * signal blocks (`8 + numSigs·16`).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SIG_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SIG_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
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

/** @brief NAV-SIG `corrSource`: correction source. */
enum class nav_sig_corr_source : uint8_t
{
    none = 0U,
    sbas = 1U,
    beidou = 2U,
    rtcm2 = 3U,
    rtcm3_osr = 4U,
    rtcm3_ssr = 5U,
    qzss_slas = 6U,
    spartn = 7U
};

/** @brief NAV-SIG `ionoModel`: ionospheric model used. */
enum class nav_sig_iono_model : uint8_t
{
    none = 0U,
    klobuchar_gps = 1U,
    sbas = 2U,
    klobuchar_beidou = 3U,
    dual_frequency = 8U ///< Iono delay derived from dual frequency observations.
};

/** @brief One signal block of UBX-NAV-SIG. */
struct nav_sig_info
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 16U;

    /** @name `sigFlags` bits @{ */
    static CASTLE_CONSTEXPR uint16_t SIG_FLAGS_HEALTH = 0x0003U;
    static CASTLE_CONSTEXPR uint16_t SIG_FLAGS_PR_SMOOTHED = 0x0004U;
    static CASTLE_CONSTEXPR uint16_t SIG_FLAGS_PR_USED = 0x0008U;
    static CASTLE_CONSTEXPR uint16_t SIG_FLAGS_CR_USED = 0x0010U;
    static CASTLE_CONSTEXPR uint16_t SIG_FLAGS_DO_USED = 0x0020U;
    static CASTLE_CONSTEXPR uint16_t SIG_FLAGS_PR_CORR_USED = 0x0040U;
    static CASTLE_CONSTEXPR uint16_t SIG_FLAGS_CR_CORR_USED = 0x0080U;
    static CASTLE_CONSTEXPR uint16_t SIG_FLAGS_DO_CORR_USED = 0x0100U;
    /** @} */

    uint8_t gnss_id = 0U; ///< GNSS identifier (see Satellite Numbering).
    uint8_t sv_id = 0U;   ///< Satellite identifier (see Satellite Numbering).
    uint8_t sig_id = 0U;  ///< New style signal identifier (see Signal Identifiers).
    uint8_t freq_id = 0U; ///< GLONASS only: frequency slot + 7 (0..13).
    int16_t pr_res = 0;   ///< Pseudorange residual [0.1 m].
    uint8_t cno = 0U;     ///< Carrier-to-noise density ratio [dBHz].
    nav_signal_quality qual_ind = nav_signal_quality::no_signal; ///< `qualityInd`; 6 and 7 equal 5.
    nav_sig_corr_source corr_source = nav_sig_corr_source::none;
    nav_sig_iono_model iono_model = nav_sig_iono_model::none;
    uint16_t sig_flags = 0U; ///< Bitmask, see `SIG_FLAGS_*`.

    CASTLE_NODISCARD nav_sv_health health() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<nav_sv_health>(sig_flags & SIG_FLAGS_HEALTH);
    }

    CASTLE_NODISCARD bool pr_smoothed() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sig_flags & SIG_FLAGS_PR_SMOOTHED) != 0U;
    }

    CASTLE_NODISCARD bool pr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sig_flags & SIG_FLAGS_PR_USED) != 0U;
    }

    CASTLE_NODISCARD bool cr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sig_flags & SIG_FLAGS_CR_USED) != 0U;
    }

    CASTLE_NODISCARD bool do_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sig_flags & SIG_FLAGS_DO_USED) != 0U;
    }

    CASTLE_NODISCARD bool pr_corr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sig_flags & SIG_FLAGS_PR_CORR_USED) != 0U;
    }

    CASTLE_NODISCARD bool cr_corr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sig_flags & SIG_FLAGS_CR_CORR_USED) != 0U;
    }

    CASTLE_NODISCARD bool do_corr_used() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (sig_flags & SIG_FLAGS_DO_CORR_USED) != 0U;
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
        stream.field(self.sig_id);
        stream.field(self.freq_id);
        stream.field(self.pr_res);
        stream.field(self.cno);
        stream.field(self.qual_ind);
        stream.field(self.corr_source);
        stream.field(self.iono_model);
        stream.field(self.sig_flags);
        stream.reserved(4U);
    }
};

/** @brief UBX-NAV-SIG output payload. */
template <castle::size_type MaxSignals = 64U>
struct nav_sig
{
    static_assert(MaxSignals <= 255U, "numSigs is a U1 field");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_SIG;
    static CASTLE_CONSTEXPR castle::size_type header_length = 8U;
    static CASTLE_CONSTEXPR castle::size_type block_length = nav_sig_info::wire_size;

    uint32_t i_tow = 0U;   ///< GPS time of week of the navigation epoch [ms].
    uint8_t version = 0U;  ///< Message version (0x00 for this version).
    uint8_t num_sigs = 0U; ///< On-wire signal count; may exceed `signals.size()`.
    castle::container::vector<nav_sig_info, MaxSignals> signals{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxSignals` signal blocks.
     * @return `false` unless the payload length is exactly `8 + numSigs·16`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_sig& out) CASTLE_NOEXCEPT
    {
        out = nav_sig{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.num_sigs, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.num_sigs, block_length))
        {
            out = nav_sig{};
            return false;
        }

        detail::read_group(reader, out.num_sigs, out.signals);
        return reader.ok();
    }

    /** @brief Writes the header and every stored block (`numSigs` is taken from `signals.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t count = static_cast<uint8_t>(signals.size());
        layout_header(*this, count, writer);
        detail::write_group(writer, signals);
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

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SIG_HPP
