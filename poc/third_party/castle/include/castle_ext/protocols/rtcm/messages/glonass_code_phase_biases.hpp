// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file glonass_code_phase_biases.hpp
 * @brief RTCM 3 message 1230: GLONASS L1 and L2 code-phase biases.
 *
 * Lets a rover align GLONASS observations of receivers from different
 * manufacturers. `signals_mask` selects which of the four biases are present
 * on the wire; biases whose mask bit is clear are not written.
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_GLONASS_CODE_PHASE_BIASES_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_GLONASS_CODE_PHASE_BIASES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"

#include "castle_ext/protocols/rtcm/bit_writer.hpp"
#include "castle_ext/protocols/rtcm/rtcm.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace rtcm
{
namespace messages
{

/** @brief RTCM message 1230. */
struct glonass_code_phase_biases
{
    /** @brief Message number (DF002). */
    static CASTLE_CONSTEXPR uint16_t MESSAGE_TYPE = RTCM_TYPE_GLO_BIASES;

    /** @name `signals_mask` bits (DF422) @{ */
    static CASTLE_CONSTEXPR uint8_t MASK_L1_CA = 0x08U;
    static CASTLE_CONSTEXPR uint8_t MASK_L1_P = 0x04U;
    static CASTLE_CONSTEXPR uint8_t MASK_L2_CA = 0x02U;
    static CASTLE_CONSTEXPR uint8_t MASK_L2_P = 0x01U;
    /** @} */

    /** @brief Resolution of the bias fields, in metres. */
    static CASTLE_CONSTEXPR double RESOLUTION_M = 0.02;

    uint16_t station_id = 0U;               ///< DF003: reference station ID (12 bit).
    bool code_phase_bias_indicator = false; ///< DF421: true when the observations of the station are aligned to GLONASS time.
    uint8_t signals_mask = 0U;              ///< DF422 (4 bit): which biases are present, see `MASK_*`.
    int16_t l1_ca_bias = 0;                 ///< DF423: L1 C/A code-phase bias, 0.02 m.
    int16_t l1_p_bias = 0;                  ///< DF424: L1 P code-phase bias, 0.02 m.
    int16_t l2_ca_bias = 0;                 ///< DF425: L2 C/A code-phase bias, 0.02 m.
    int16_t l2_p_bias = 0;                  ///< DF426: L2 P code-phase bias, 0.02 m.

    /** @brief Reports whether `type` is 1230. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR bool accepts(uint16_t type) CASTLE_NOEXCEPT
    {
        return type == MESSAGE_TYPE;
    }

    /** @brief Writes the message payload; see `encode_message()`. */
    void encode(bit_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        writer.field(MESSAGE_TYPE, RTCM_MESSAGE_NUMBER_BITS);
        writer.field(station_id, RTCM_STATION_ID_BITS);
        writer.field(code_phase_bias_indicator);
        writer.reserved(3U);
        writer.field(signals_mask, 4U);
        if ((signals_mask & MASK_L1_CA) != 0U)
        {
            writer.field(l1_ca_bias, 16U);
        }
        if ((signals_mask & MASK_L1_P) != 0U)
        {
            writer.field(l1_p_bias, 16U);
        }
        if ((signals_mask & MASK_L2_CA) != 0U)
        {
            writer.field(l2_ca_bias, 16U);
        }
        if ((signals_mask & MASK_L2_P) != 0U)
        {
            writer.field(l2_p_bias, 16U);
        }
    }
};

} // namespace messages
} // namespace rtcm
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_GLONASS_CODE_PHASE_BIASES_HPP
