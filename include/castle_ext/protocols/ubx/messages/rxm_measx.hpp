// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_MEASX_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_MEASX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief One decoded satellite measurement block from UBX-RXM-MEASX. */
struct rxm_measx_sv
{
    uint8_t gnss_id = 0U;
    uint8_t sv_id = 0U;
    uint8_t cno = 0U;
    uint8_t mpath_indic = 0U;
    int32_t doppler_ms = 0;
    int32_t doppler_hz = 0;
    uint16_t whole_chips = 0U;
    uint16_t frac_chips = 0U;
    uint32_t code_phase = 0U;
    uint8_t int_code_phase = 0U;
    uint8_t pseu_range_rms_err = 0U;
};

/** @brief UBX-RXM-MEASX: satellite measurements for signal/time correlation. */
template <castle::size_type MaxSvs = 64U>
struct rxm_measx
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_RXM;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_RXM_MEASX;
    static CASTLE_CONSTEXPR castle::size_type header_length = 44U;
    static CASTLE_CONSTEXPR castle::size_type block_length = 24U;

    uint8_t version = 0U;
    uint32_t gps_tow = 0U;
    uint32_t glo_tow = 0U;
    uint32_t bds_tow = 0U;
    uint32_t qzss_tow = 0U;
    uint16_t gps_tow_acc = 0U;
    uint16_t glo_tow_acc = 0U;
    uint16_t bds_tow_acc = 0U;
    uint16_t qzss_tow_acc = 0U;
    uint8_t num_svs = 0U;
    uint8_t flags = 0U;
    castle::container::array<rxm_measx_sv, MaxSvs> svs{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, rxm_measx& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < header_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = rxm_measx{};
        out.version = reader.read_u8();
        reader.skip(3U);
        out.gps_tow = reader.read_u32();
        out.glo_tow = reader.read_u32();
        out.bds_tow = reader.read_u32();
        reader.skip(4U);
        out.qzss_tow = reader.read_u32();
        out.gps_tow_acc = reader.read_u16();
        out.glo_tow_acc = reader.read_u16();
        out.bds_tow_acc = reader.read_u16();
        reader.skip(2U);
        out.qzss_tow_acc = reader.read_u16();
        out.num_svs = reader.read_u8();
        out.flags = reader.read_u8();
        reader.skip(8U);

        castle::size_type expected = header_length + (static_cast<castle::size_type>(out.num_svs) * block_length);
        if (raw.payload_length() != expected)
        {
            return false;
        }

        castle::size_type count = (out.num_svs < MaxSvs) ? out.num_svs : MaxSvs;
        for (castle::size_type index = 0U; index < count; ++index)
        {
            auto& sv = out.svs[index];
            sv.gnss_id = reader.read_u8();
            sv.sv_id = reader.read_u8();
            sv.cno = reader.read_u8();
            sv.mpath_indic = reader.read_u8();
            sv.doppler_ms = reader.read_i32();
            sv.doppler_hz = reader.read_i32();
            sv.whole_chips = reader.read_u16();
            sv.frac_chips = reader.read_u16();
            sv.code_phase = reader.read_u32();
            sv.int_code_phase = reader.read_u8();
            sv.pseu_range_rms_err = reader.read_u8();
            reader.skip(2U);
        }
        if (out.num_svs > count)
        {
            reader.skip((static_cast<castle::size_type>(out.num_svs) - count) * block_length);
        }

        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_MEASX_HPP
