// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_STATUS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_STATUS_HPP

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

/** @brief One decoded sensor status block from UBX-ESF-STATUS. */
struct esf_status_sensor
{
    static CASTLE_CONSTEXPR uint8_t TYPE_MASK = 0x1FU;
    static CASTLE_CONSTEXPR uint8_t USED = 0x20U;
    static CASTLE_CONSTEXPR uint8_t READY = 0x40U;
    static CASTLE_CONSTEXPR uint8_t CALIB_STATUS_MASK = 0x03U;
    static CASTLE_CONSTEXPR uint8_t TIME_STATUS_MASK = 0x0CU;

    uint8_t sens_status1 = 0U;
    uint8_t sens_status2 = 0U;
    uint8_t freq = 0U;
    uint8_t faults = 0U;
};

/** @brief UBX-ESF-STATUS: sensor fusion status and per-sensor diagnostics. */
template <castle::size_type MaxSensors = 32U>
struct esf_status
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_ESF;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_ESF_STATUS;
    static CASTLE_CONSTEXPR castle::size_type min_payload_length = 16U;

    uint32_t i_tow = 0U;
    uint8_t version = 0U;
    uint8_t fusion_mode = 0U;
    uint8_t num_sens = 0U;
    castle::container::array<esf_status_sensor, MaxSensors> sensors{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, esf_status& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < min_payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = esf_status{};
        out.i_tow = reader.read_u32();
        out.version = reader.read_u8();
        reader.skip(7U);
        out.fusion_mode = reader.read_u8();
        reader.skip(2U);
        out.num_sens = reader.read_u8();

        castle::size_type count = (out.num_sens < MaxSensors) ? out.num_sens : MaxSensors;
        castle::size_type expected = min_payload_length + (4U * static_cast<castle::size_type>(out.num_sens));
        if (raw.payload_length() < expected)
        {
            return false;
        }

        for (castle::size_type index = 0U; index < count; ++index)
        {
            out.sensors[index].sens_status1 = reader.read_u8();
            out.sensors[index].sens_status2 = reader.read_u8();
            out.sensors[index].freq = reader.read_u8();
            out.sensors[index].faults = reader.read_u8();
        }
        if (out.num_sens > count)
        {
            reader.skip((static_cast<castle::size_type>(out.num_sens) - count) * 4U);
        }

        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_STATUS_HPP
