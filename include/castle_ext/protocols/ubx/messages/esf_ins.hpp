// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_INS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_INS_HPP

#include "castle/core/compiler.hpp"
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

/** @brief UBX-ESF-INS: fused IMU angular rate and acceleration solution. */
struct esf_ins
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_ESF;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_ESF_INS;
    static CASTLE_CONSTEXPR castle::size_type payload_length = 36U;

    static CASTLE_CONSTEXPR uint32_t BF0_X_ANG_RATE_VALID = 1U;
    static CASTLE_CONSTEXPR uint32_t BF0_Y_ANG_RATE_VALID = 2U;
    static CASTLE_CONSTEXPR uint32_t BF0_Z_ANG_RATE_VALID = 4U;
    static CASTLE_CONSTEXPR uint32_t BF0_X_ACCEL_VALID = 8U;
    static CASTLE_CONSTEXPR uint32_t BF0_Y_ACCEL_VALID = 16U;
    static CASTLE_CONSTEXPR uint32_t BF0_Z_ACCEL_VALID = 32U;

    uint32_t bitfield0 = 0U;
    uint32_t i_tow = 0U;
    int32_t x_ang_rate = 0;
    int32_t y_ang_rate = 0;
    int32_t z_ang_rate = 0;
    int32_t x_accel = 0;
    int32_t y_accel = 0;
    int32_t z_accel = 0;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, esf_ins& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() != payload_length)
        {
            return false;
        }

        payload_reader reader(raw.payload);
        out = esf_ins{};
        out.bitfield0 = reader.read_u32();
        reader.skip(4U);
        out.i_tow = reader.read_u32();
        out.x_ang_rate = reader.read_i32();
        out.y_ang_rate = reader.read_i32();
        out.z_ang_rate = reader.read_i32();
        out.x_accel = reader.read_i32();
        out.y_accel = reader.read_i32();
        out.z_accel = reader.read_i32();
        return reader.ok();
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_ESF_INS_HPP
