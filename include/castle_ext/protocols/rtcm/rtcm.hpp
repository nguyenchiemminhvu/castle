// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file rtcm.hpp
 * @brief RTCM 3.x transport framing primitives for Castle (encode only).
 *
 * RTCM 3 messages are sent *to* a GNSS receiver (for example a u-blox ZED-F9R)
 * to supply differential correction data. This module builds the transport
 * frame around an already encoded message payload:
 *
 * `[0xD3][6 reserved bits | 10-bit length][payload][CRC-24Q]`
 *
 * The CRC-24Q covers the preamble, the length field and the payload, and is
 * transmitted most-significant byte first. Everything is deterministic and
 * works on caller-provided buffers (no heap, exceptions, RTTI or STL).
 *
 * Message payloads themselves are produced by the message models in
 * `castle_ext/protocols/rtcm/messages/` through `encode_message()`
 * (see `bit_writer.hpp`); `encode_frame()` frames a payload you already have.
 *
 * @code
 * #include "castle_ext/protocols/rtcm/rtcm.hpp"
 *
 * uint8_t payload_data[2U] = {0x3EU, 0x90U};
 * castle::container::array_view<const uint8_t> payload(payload_data, 2U);
 *
 * uint8_t frame[16U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::rtcm::encode_frame(payload, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_RTCM_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_RTCM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array_view.hpp"
#include "castle/utility/bytes.hpp"

#include "castle_ext/checksums/crc24q.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace rtcm
{

/** @brief RTCM 3 frame preamble byte. */
static CASTLE_CONSTEXPR uint8_t RTCM_PREAMBLE = 0xD3U;

/** @brief Byte offsets inside an encoded frame: `[preamble][len_hi][len_lo][payload...][crc x3]`. */
static CASTLE_CONSTEXPR castle::size_type RTCM_FRAME_INDEX_PREAMBLE = 0U;
static CASTLE_CONSTEXPR castle::size_type RTCM_FRAME_INDEX_LENGTH = 1U;
static CASTLE_CONSTEXPR castle::size_type RTCM_FRAME_INDEX_PAYLOAD = 3U;

/** @brief Byte width of the trailing CRC-24Q. */
static CASTLE_CONSTEXPR castle::size_type RTCM_CRC_SIZE = 3U;

/** @brief Fixed byte count outside the payload (3-byte header + 3-byte CRC). */
static CASTLE_CONSTEXPR castle::size_type RTCM_FRAME_OVERHEAD = RTCM_FRAME_INDEX_PAYLOAD + RTCM_CRC_SIZE;

/** @brief Largest payload representable by the 10-bit length field. */
static CASTLE_CONSTEXPR castle::size_type RTCM_MAX_PAYLOAD_LEN = 1023U;

/** @brief Largest complete frame. */
static CASTLE_CONSTEXPR castle::size_type RTCM_MAX_FRAME_SIZE = RTCM_MAX_PAYLOAD_LEN + RTCM_FRAME_OVERHEAD;

/** @brief Bit width of the message number (DF002) that starts every payload. */
static CASTLE_CONSTEXPR castle::size_type RTCM_MESSAGE_NUMBER_BITS = 12U;

/** @brief Bit width of the reference station ID (DF003) that follows the message number. */
static CASTLE_CONSTEXPR castle::size_type RTCM_STATION_ID_BITS = 12U;

// -----------------------------------------------------------------------------
// Message numbers accepted by u-blox receivers (UBX-19056845 chapter 4)
// -----------------------------------------------------------------------------

static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_GPS_L1_OBS          = 1001U; ///< L1-only GPS RTK observables.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_GPS_L1_OBS_EXT      = 1002U; ///< Extended L1-only GPS RTK observables.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_GPS_L1L2_OBS        = 1003U; ///< L1/L2 GPS RTK observables.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_GPS_L1L2_OBS_EXT    = 1004U; ///< Extended L1/L2 GPS RTK observables.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_STATION_ARP         = 1005U; ///< Stationary reference station ARP.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_STATION_ARP_HEIGHT  = 1006U; ///< Stationary reference station ARP with antenna height.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_ANTENNA_DESCRIPTOR  = 1007U; ///< Antenna descriptor.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_GLO_L1_OBS          = 1009U; ///< L1-only GLONASS RTK observables.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_GLO_L1_OBS_EXT      = 1010U; ///< Extended L1-only GLONASS RTK observables.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_GLO_L1L2_OBS        = 1011U; ///< L1/L2 GLONASS RTK observables.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_GLO_L1L2_OBS_EXT    = 1012U; ///< Extended L1/L2 GLONASS RTK observables.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_RECEIVER_ANTENNA    = 1033U; ///< Receiver and antenna descriptors.
static CASTLE_CONSTEXPR uint16_t RTCM_TYPE_GLO_BIASES          = 1230U; ///< GLONASS L1 and L2 code-phase biases.

/**
 * @brief Returns the complete encoded frame size for a payload length.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR castle::size_type frame_size(castle::size_type payload_length) CASTLE_NOEXCEPT
{
    return payload_length + RTCM_FRAME_OVERHEAD;
}

namespace detail
{

/**
 * @brief Completes a frame whose payload is already stored at `frame[RTCM_FRAME_INDEX_PAYLOAD]`.
 *
 * Writes the preamble and the length field, then appends the CRC-24Q. Shared
 * by `encode_frame()` and `encode_message()`; performs no checks.
 *
 * @param frame Frame buffer with room for `frame_size(payload_length)` bytes.
 * @param payload_length Payload size in bytes, at most `RTCM_MAX_PAYLOAD_LEN`.
 * @return Total frame size.
 */
CASTLE_INLINE castle::size_type finish_frame(uint8_t* frame, castle::size_type payload_length) CASTLE_NOEXCEPT
{
    // The 6 reserved bits above the 10-bit length are zero because payload_length <= 1023.
    frame[RTCM_FRAME_INDEX_PREAMBLE] = RTCM_PREAMBLE;
    castle::write_be16(&frame[RTCM_FRAME_INDEX_LENGTH], static_cast<uint16_t>(payload_length));

    CASTLE_CONST castle::size_type covered = RTCM_FRAME_INDEX_PAYLOAD + payload_length;
    CASTLE_CONST uint32_t crc = castle::checksums::crc24q::calculate(frame, covered);
    frame[covered] = static_cast<uint8_t>(crc >> 16U);
    frame[covered + 1U] = static_cast<uint8_t>(crc >> 8U);
    frame[covered + 2U] = static_cast<uint8_t>(crc);
    return covered + RTCM_CRC_SIZE;
}

} // namespace detail

/**
 * @brief Frames an already encoded RTCM payload into a caller-provided buffer.
 *
 * @param payload Message payload (message number, data fields and zero padding to a byte boundary).
 * @param output Destination frame buffer.
 * @param output_capacity Size of the destination buffer.
 * @param[out] bytes_written Frame size on success, otherwise zero.
 * @return `ok`; `invalid_argument` for a null `output` or a null non-empty payload;
 * `out_of_range` when the payload exceeds `RTCM_MAX_PAYLOAD_LEN`;
 * `full` when the frame does not fit.
 *
 * @note Never allocates and never writes outside `output`.
 */
CASTLE_NODISCARD CASTLE_INLINE castle::status encode_frame(
    castle::container::array_view<CASTLE_CONST uint8_t> payload,
    uint8_t* output,
    castle::size_type output_capacity,
    castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;

    if ((output == nullptr) || ((payload.size() > 0U) && (payload.data() == nullptr)))
    {
        return castle::status::invalid_argument;
    }
    if (payload.size() > RTCM_MAX_PAYLOAD_LEN)
    {
        return castle::status::out_of_range;
    }
    if (output_capacity < frame_size(payload.size()))
    {
        return castle::status::full;
    }

    for (castle::size_type index = 0U; index < payload.size(); ++index)
    {
        output[RTCM_FRAME_INDEX_PAYLOAD + index] = payload[index];
    }

    bytes_written = detail::finish_frame(output, payload.size());
    return castle::status::ok;
}

} // namespace rtcm
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_RTCM_RTCM_HPP
