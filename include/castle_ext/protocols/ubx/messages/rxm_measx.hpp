// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file rxm_measx.hpp
 * @brief UBX-RXM-MEASX (0x02 0x14): satellite measurements for RRLP.
 *
 * Periodic/polled output: a 44-byte header followed by `numSV` 24-byte
 * satellite blocks (`44 + numSV·24`). Payload data follows the RRLP
 * (3GPP TS 44.031) conventions, except that GNSS/satellite IDs use the
 * u-blox Satellite Numbering scheme.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_MEASX_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_MEASX_HPP

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

/** @brief `mpathIndic`: RRLP multipath indicator. */
enum class rxm_measx_mpath : uint8_t
{
    not_measured = 0U,
    low = 1U,
    medium = 2U,
    high = 3U
};

/** @brief One satellite block of UBX-RXM-MEASX. */
struct rxm_measx_sv
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 24U;

    uint8_t gnss_id = 0U;            ///< GNSS identifier (see Satellite Numbering).
    uint8_t sv_id = 0U;              ///< Satellite identifier (see Satellite Numbering).
    uint8_t cno = 0U;                ///< Carrier-to-noise ratio (0..63).
    uint8_t mpath_indic = 0U;        ///< Multipath indicator, see `rxm_measx_mpath`.
    int32_t doppler_ms = 0;          ///< Doppler measurement [0.04 m/s].
    int32_t doppler_hz = 0;          ///< Doppler measurement [0.2 Hz].
    uint16_t whole_chips = 0U;       ///< Whole value of the code phase (0..1022 for GPS).
    uint16_t frac_chips = 0U;        ///< Fractional value of the code phase (0..1023).
    uint32_t code_phase = 0U;        ///< Code phase [2^-21 ms].
    uint8_t int_code_phase = 0U;     ///< Integer part of the code phase [ms].
    uint8_t pseu_range_rms_err = 0U; ///< RRLP pseudorange RMS error index (0..63).

    CASTLE_NODISCARD rxm_measx_mpath mpath() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<rxm_measx_mpath>(mpath_indic);
    }

    /** @brief Doppler measurement [m/s]. */
    CASTLE_NODISCARD double doppler_speed_mps() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(doppler_ms) * 4.0 * castle::math::pow10< -2, double>();
    }

    /** @brief Doppler measurement [Hz]. */
    CASTLE_NODISCARD double doppler_frequency_hz() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(doppler_hz) * 2.0 * castle::math::pow10< -1, double>();
    }

    /** @brief Code phase [ms]. */
    CASTLE_NODISCARD double code_phase_ms() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<double>(code_phase) * CODE_PHASE_SCALE;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.gnss_id);
        stream.field(self.sv_id);
        stream.field(self.cno);
        stream.field(self.mpath_indic);
        stream.field(self.doppler_ms);
        stream.field(self.doppler_hz);
        stream.field(self.whole_chips);
        stream.field(self.frac_chips);
        stream.field(self.code_phase);
        stream.field(self.int_code_phase);
        stream.field(self.pseu_range_rms_err);
        stream.reserved(2U);
    }

private:
    /** @brief 2^-21, the `codePhase` scale (exact in `double`). */
    static CASTLE_CONSTEXPR double CODE_PHASE_SCALE = 1.0 / 2097152.0;
};

/** @brief UBX-RXM-MEASX output payload. */
template <castle::size_type MaxSvs = 64U>
struct rxm_measx
{
    static_assert(MaxSvs <= 255U, "numSV is a U1 field");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_RXM;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_RXM_MEASX;
    static CASTLE_CONSTEXPR castle::size_type header_length = 44U;
    static CASTLE_CONSTEXPR castle::size_type block_length = rxm_measx_sv::wire_size;

    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_TOW_SET = 0x03U; ///< `towSet`: 0 = no, 1 or 2 = yes.
    /** @} */

    /** @brief `*_tow_acc` value meaning "> 4 s". */
    static CASTLE_CONSTEXPR uint16_t TOW_ACC_OVER_4S = 0xFFFFU;

    uint8_t version = 0U;        ///< Message version (0x01 for this version).
    uint32_t gps_tow = 0U;       ///< GPS measurement reference time [ms].
    uint32_t glo_tow = 0U;       ///< GLONASS measurement reference time [ms].
    uint32_t bds_tow = 0U;       ///< BeiDou measurement reference time [ms].
    uint32_t qzss_tow = 0U;      ///< QZSS measurement reference time [ms].
    uint16_t gps_tow_acc = 0U;   ///< GPS reference time accuracy [2^-4 ms].
    uint16_t glo_tow_acc = 0U;   ///< GLONASS reference time accuracy [2^-4 ms].
    uint16_t bds_tow_acc = 0U;   ///< BeiDou reference time accuracy [2^-4 ms].
    uint16_t qzss_tow_acc = 0U;  ///< QZSS reference time accuracy [2^-4 ms].
    uint8_t num_svs = 0U;        ///< On-wire satellite count (`numSV`); may exceed `svs.size()`.
    uint8_t flags = 0U;          ///< Flags, see `FLAGS_*`.
    castle::container::vector<rxm_measx_sv, MaxSvs> svs{};

    /** @brief Raw `towSet` value (0 = not set, 1 or 2 = set). */
    CASTLE_NODISCARD uint8_t tow_set() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(flags & FLAGS_TOW_SET);
    }

    /** @brief Converts a raw `*_tow_acc` field (2^-4 ms) to milliseconds. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR double tow_acc_ms(uint16_t raw_acc) CASTLE_NOEXCEPT
    {
        return static_cast<double>(raw_acc) * 0.0625;
    }

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxSvs` satellite blocks.
     * @return `false` unless the payload length is exactly `44 + numSV·24`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, rxm_measx& out) CASTLE_NOEXCEPT
    {
        out = rxm_measx{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.num_svs, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.num_svs, block_length))
        {
            out = rxm_measx{};
            return false;
        }

        detail::read_group(reader, out.num_svs, out.svs);
        return reader.ok();
    }

    /** @brief Writes the header and every stored block (`numSV` is taken from `svs.size()`). */
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
        stream.field(self.version);
        stream.reserved(3U);
        stream.field(self.gps_tow);
        stream.field(self.glo_tow);
        stream.field(self.bds_tow);
        stream.reserved(4U);
        stream.field(self.qzss_tow);
        stream.field(self.gps_tow_acc);
        stream.field(self.glo_tow_acc);
        stream.field(self.bds_tow_acc);
        stream.reserved(2U);
        stream.field(self.qzss_tow_acc);
        stream.field(count);
        stream.field(self.flags);
        stream.reserved(8U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_MEASX_HPP
