// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_MEAS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_MEAS_HPP

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

/** @brief One decoded external sensor measurement from UBX-ESF-MEAS. */
struct esf_meas_datum
{
    uint8_t data_type = 0U;
    int32_t data_value = 0;
};

/** @brief UBX-ESF-MEAS: external sensor fusion measurement data. */
template <castle::size_type MaxData = 32U>
struct esf_meas
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_ESF;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_ESF_MEAS;
    static CASTLE_CONSTEXPR castle::size_type min_payload_length = 8U;
    static CASTLE_CONSTEXPR uint16_t FLAGS_TIME_TAG_TYPE_MASK = 0x0003U;
    static CASTLE_CONSTEXPR uint16_t FLAGS_TIME_MARK_SENT_MASK = 0x0018U;

    uint32_t time_tag = 0U;
    uint16_t flags = 0U;
    uint16_t id = 0U;
    uint8_t num_meas = 0U;
    castle::container::array<esf_meas_datum, MaxData> data{};
    bool has_calib_ttag = false;
    uint32_t calib_ttag = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, esf_meas& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < min_payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = esf_meas{};
        out.time_tag = reader.read_u32();
        out.flags = reader.read_u16();
        out.id = reader.read_u16();

        bool has_calib = ((out.flags & FLAGS_TIME_TAG_TYPE_MASK) != 0U);
        castle::size_type data_bytes = raw.payload_length() - min_payload_length;
        castle::size_type calib_bytes = has_calib ? 4U : 0U;
        if (data_bytes < calib_bytes)
        {
            return false;
        }

        castle::size_type meas_bytes = data_bytes - calib_bytes;
        if ((meas_bytes % 4U) != 0U)
        {
            return false;
        }

        castle::size_type count = meas_bytes / 4U;
        castle::size_type decode_count = (count < MaxData) ? count : MaxData;
        out.num_meas = static_cast<uint8_t>(decode_count);
        for (castle::size_type index = 0U; index < decode_count; ++index)
        {
            uint32_t word = reader.read_u32();
            out.data[index].data_type = static_cast<uint8_t>((word >> 24U) & 0x3FU);
            uint32_t raw_value = word & 0x00FFFFFFU;
            out.data[index].data_value = (raw_value & 0x00800000U)
                ? static_cast<int32_t>(raw_value | 0xFF000000U)
                : static_cast<int32_t>(raw_value);
        }
        if (count > decode_count)
        {
            reader.skip((count - decode_count) * 4U);
        }

        out.has_calib_ttag = has_calib;
        if (has_calib)
        {
            out.calib_ttag = reader.read_u32();
        }

        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_MEAS_HPP
