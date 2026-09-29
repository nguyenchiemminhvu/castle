// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SAT_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SAT_HPP

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

/** @brief One decoded satellite block from UBX-NAV-SAT. */
struct nav_sat_sv
{
    uint8_t gnss_id = 0U;
    uint8_t sv_id = 0U;
    uint8_t cno = 0U;
    int8_t elev = 0;
    int16_t azim = 0;
    int16_t pr_res = 0;
    uint32_t flags = 0U;
};

/** @brief UBX-NAV-SAT: satellite information. */
template <castle::size_type MaxSvs = 64U>
struct nav_sat
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_SAT;
    static CASTLE_CONSTEXPR castle::size_type header_length = 8U;
    static CASTLE_CONSTEXPR castle::size_type block_length = 12U;

    uint32_t i_tow = 0U;
    uint8_t version = 0U;
    uint8_t num_svs = 0U;
    castle::container::array<nav_sat_sv, MaxSvs> svs{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_sat& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < header_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = nav_sat{};
        out.i_tow = reader.read_u32();
        out.version = reader.read_u8();
        out.num_svs = reader.read_u8();
        reader.skip(2U);

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
            sv.elev = reader.read_i8();
            sv.azim = reader.read_i16();
            sv.pr_res = reader.read_i16();
            sv.flags = reader.read_u32();
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

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_SAT_HPP
