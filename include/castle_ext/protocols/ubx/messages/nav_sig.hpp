// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SIG_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SIG_HPP

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

/** @brief One decoded signal block from UBX-NAV-SIG. */
struct nav_sig_info
{
    uint8_t gnss_id = 0U;
    uint8_t sv_id = 0U;
    uint8_t sig_id = 0U;
    uint8_t freq_id = 0U;
    int16_t pr_res = 0;
    uint8_t cno = 0U;
    uint8_t qual_ind = 0U;
    uint8_t corr_source = 0U;
    uint8_t iono_model = 0U;
    uint16_t sig_flags = 0U;
};

/** @brief UBX-NAV-SIG: signal information. */
template <castle::size_type MaxSignals = 64U>
struct nav_sig
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_SIG;
    static CASTLE_CONSTEXPR castle::size_type header_length = 8U;
    static CASTLE_CONSTEXPR castle::size_type block_length = 16U;

    uint32_t i_tow = 0U;
    uint8_t version = 0U;
    uint8_t num_sigs = 0U;
    castle::container::array<nav_sig_info, MaxSignals> signals{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_sig& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < header_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = nav_sig{};
        out.i_tow = reader.read_u32();
        out.version = reader.read_u8();
        out.num_sigs = reader.read_u8();
        reader.skip(2U);

        castle::size_type expected = header_length + (static_cast<castle::size_type>(out.num_sigs) * block_length);
        if (raw.payload_length() != expected)
        {
            return false;
        }

        castle::size_type count = (out.num_sigs < MaxSignals) ? out.num_sigs : MaxSignals;
        for (castle::size_type index = 0U; index < count; ++index)
        {
            auto& signal = out.signals[index];
            signal.gnss_id = reader.read_u8();
            signal.sv_id = reader.read_u8();
            signal.sig_id = reader.read_u8();
            signal.freq_id = reader.read_u8();
            signal.pr_res = reader.read_i16();
            signal.cno = reader.read_u8();
            signal.qual_ind = reader.read_u8();
            signal.corr_source = reader.read_u8();
            signal.iono_model = reader.read_u8();
            signal.sig_flags = reader.read_u16();
            reader.skip(4U);
        }
        if (out.num_sigs > count)
        {
            reader.skip((static_cast<castle::size_type>(out.num_sigs) - count) * block_length);
        }

        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SIG_HPP
