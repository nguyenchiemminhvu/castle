// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ubx.hpp
 * @brief Header-only UBX binary protocol primitives for Castle.
 *
 * This module contains only protocol-level data and algorithms:
 *  - UBX framing constants and well-known message class/ID identifiers;
 *  - shared endian-explicit byte helpers from `castle/utility/bytes.hpp`;
 *  - Fletcher-8 checksum calculation;
 *  - non-owning decoded message views;
 *  - deterministic, caller-provided-buffer frame encoding and validation.
 *
 * @code
 * #include "castle_ext/protocols/ubx/ubx.hpp"
 *
 * uint8_t frame[64U];
 * uint8_t payload_data[2U] = {0x01U, 0x02U};
 * castle::container::array_view<const uint8_t> payload(payload_data, 2U);
 *
 * castle::size_type written = 0U;
 * castle::protocols::ubx::encode_frame(
 *     castle::protocols::ubx::UBX_CLASS_CFG,
 *     castle::protocols::ubx::UBX_ID_CFG_VALSET,
 *     payload,
 *     frame,
 *     sizeof(frame),
 *     written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_UBX_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_UBX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/container/array_view.hpp"
#include "castle/utility/bytes.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{

/**
 * @brief UBX protocol synchronization bytes.
 */
static CASTLE_CONSTEXPR uint8_t UBX_SYNC_CHAR_1 = 0xB5U;
static CASTLE_CONSTEXPR uint8_t UBX_SYNC_CHAR_2 = 0x62U;

/**
 * @brief Number of bits in one byte, used for little-endian shift/mask math.
 */
static CASTLE_CONSTEXPR castle::size_type UBX_BITS_PER_BYTE = 8U;

/**
 * @brief Mask isolating the low 8 bits of a wider integer.
 */
static CASTLE_CONSTEXPR uint32_t UBX_BYTE_MASK = 0xFFU;

/**
 * @brief Byte offsets of the fixed-position fields in an encoded UBX frame.
 *
 * A UBX frame is:
 * `[SYNC1][SYNC2][CLASS][ID][LEN_LO][LEN_HI][PAYLOAD...][CK_A][CK_B]`.
 */
static CASTLE_CONSTEXPR castle::size_type UBX_FRAME_INDEX_SYNC_1 = 0U;
static CASTLE_CONSTEXPR castle::size_type UBX_FRAME_INDEX_SYNC_2 = 1U;
static CASTLE_CONSTEXPR castle::size_type UBX_FRAME_INDEX_CLASS = 2U;
static CASTLE_CONSTEXPR castle::size_type UBX_FRAME_INDEX_ID = 3U;
static CASTLE_CONSTEXPR castle::size_type UBX_FRAME_INDEX_LENGTH = 4U;
static CASTLE_CONSTEXPR castle::size_type UBX_FRAME_INDEX_PAYLOAD = 6U;

/**
 * @brief Byte width of the trailing Fletcher-8 checksum pair (CK_A, CK_B).
 */
static CASTLE_CONSTEXPR castle::size_type UBX_CHECKSUM_SIZE = 2U;

/**
 * @brief Fixed byte count outside the payload.
 */
static CASTLE_CONSTEXPR castle::size_type UBX_FRAME_OVERHEAD = UBX_FRAME_INDEX_PAYLOAD + UBX_CHECKSUM_SIZE;

/**
 * @brief Maximum payload representable by the UBX 16-bit length field.
 */
static CASTLE_CONSTEXPR uint16_t UBX_MAX_PAYLOAD_LEN = 0xFFFFU;

/**
 * @brief Default practical payload ceiling for deterministic embedded parsing.
 */
static CASTLE_CONSTEXPR uint16_t UBX_SAFE_MAX_PAYLOAD_LEN = 4096U;

// -----------------------------------------------------------------------------
// UBX message classes
// -----------------------------------------------------------------------------

static CASTLE_CONSTEXPR uint8_t UBX_CLASS_NAV  = 0x01U;
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_RXM  = 0x02U;
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_INF  = 0x04U;
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_ACK  = 0x05U;
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_CFG  = 0x06U;
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_UPD  = 0x09U;
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_MON  = 0x0AU;
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_TIM  = 0x0DU;
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_ESF  = 0x10U;
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_SEC  = 0x27U;
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_NAV2 = 0x29U;

// -----------------------------------------------------------------------------
// UBX message IDs
// -----------------------------------------------------------------------------

// NAV (0x01)
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_STATUS  = 0x03U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_DOP     = 0x04U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_ATT     = 0x05U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_PVT     = 0x07U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_ODO     = 0x09U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_TIMEGPS = 0x20U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_TIMEUTC = 0x21U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_CLOCK   = 0x22U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_SAT     = 0x35U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_EELL    = 0x3DU;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV_SIG     = 0x43U;

// RXM (0x02)
static CASTLE_CONSTEXPR uint8_t UBX_ID_RXM_MEASX = 0x14U;

// INF (0x04)
static CASTLE_CONSTEXPR uint8_t UBX_ID_INF_ERROR   = 0x00U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_INF_WARNING = 0x01U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_INF_NOTICE  = 0x02U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_INF_TEST    = 0x03U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_INF_DEBUG   = 0x04U;

// ACK (0x05)
static CASTLE_CONSTEXPR uint8_t UBX_ID_ACK_NAK = 0x00U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_ACK_ACK = 0x01U;

// CFG (0x06)
static CASTLE_CONSTEXPR uint8_t UBX_CLASS_CFG_RST = UBX_CLASS_CFG;
static CASTLE_CONSTEXPR uint8_t UBX_ID_CFG_RST    = 0x04U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_CFG_VALSET = 0x8AU;
static CASTLE_CONSTEXPR uint8_t UBX_ID_CFG_VALGET = 0x8BU;

// UPD (0x09)
static CASTLE_CONSTEXPR uint8_t UBX_ID_UPD_SOS = 0x14U;

// MON (0x0A)
static CASTLE_CONSTEXPR uint8_t UBX_ID_MON_IO    = 0x02U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_MON_VER   = 0x04U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_MON_TXBUF = 0x08U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_MON_SPAN  = 0x31U;

// TIM (0x0D)
static CASTLE_CONSTEXPR uint8_t UBX_ID_TIM_TP = 0x01U;

// ESF (0x10)
static CASTLE_CONSTEXPR uint8_t UBX_ID_ESF_MEAS   = 0x02U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_ESF_STATUS = 0x10U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_ESF_INS    = 0x15U;

// SEC (0x27)
static CASTLE_CONSTEXPR uint8_t UBX_ID_SEC_CRC = 0x01U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_SEC_SIG = 0x09U;

// NAV2 (0x29)
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV2_DOP     = 0x04U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV2_PVT     = 0x07U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV2_TIMEGPS = 0x20U;
static CASTLE_CONSTEXPR uint8_t UBX_ID_NAV2_EELL    = 0x3DU;

// -----------------------------------------------------------------------------
// Message-specific protocol constants used by the reference implementation.
// -----------------------------------------------------------------------------

static CASTLE_CONSTEXPR uint8_t UBX_VALSET_VERSION      = 0x00U;
static CASTLE_CONSTEXPR uint8_t UBX_VALGET_VERSION_POLL = 0x00U;
static CASTLE_CONSTEXPR uint8_t UBX_VALGET_VERSION_RESP = 0x01U;

static CASTLE_CONSTEXPR uint8_t CFG_RST_MODE_CTRL_SW          = 0x01U;
static CASTLE_CONSTEXPR uint8_t CFG_RST_MODE_CTRL_SW_GNSS_ONLY = 0x02U;
static CASTLE_CONSTEXPR uint8_t CFG_RST_MODE_CTRL_STOP        = 0x08U;
static CASTLE_CONSTEXPR uint8_t CFG_RST_MODE_CTRL_START       = 0x09U;

static CASTLE_CONSTEXPR uint16_t NAV_BBR_HOT_START  = 0x0000U;
static CASTLE_CONSTEXPR uint16_t NAV_BBR_WARM_START = 0x0001U;
static CASTLE_CONSTEXPR uint16_t NAV_BBR_COLD_START = 0xFFFFU;

static CASTLE_CONSTEXPR uint8_t UPD_SOS_CMD_SAVE    = 0x00U;
static CASTLE_CONSTEXPR uint8_t UPD_SOS_CMD_CLEAR   = 0x01U;
static CASTLE_CONSTEXPR uint8_t UPD_SOS_CMD_ACK     = 0x02U;
static CASTLE_CONSTEXPR uint8_t UPD_SOS_CMD_RESTORE = 0x03U;
static CASTLE_CONSTEXPR uint8_t UPD_SOS_RESP_NOT_ACK = 0x00U;
static CASTLE_CONSTEXPR uint8_t UPD_SOS_RESP_ACK     = 0x01U;

/**
 * @brief Decoded UBX header.
 */
struct message_header
{
    uint8_t  msg_class = 0U;
    uint8_t  msg_id = 0U;
    uint16_t payload_length = 0U;
};

/**
 * @brief UBX checksum pair.
 */
struct checksum
{
    uint8_t ck_a = 0U;
    uint8_t ck_b = 0U;
};

/**
 * @brief Non-owning view of a fully validated UBX message.
 *
 * The payload is owned by the producer of the view. In the parser this is
 * the parser's fixed-capacity inline payload buffer. The view is only valid
 * while that storage remains unchanged.
 */
struct message_view
{
    message_header header{};
    castle::container::array_view<CASTLE_CONST uint8_t> payload{};
    checksum check{};

    /**
     * @brief Returns the message class.
     */
    CASTLE_NODISCARD uint8_t msg_class() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return header.msg_class;
    }

    /**
     * @brief Returns the message ID.
     */
    CASTLE_NODISCARD uint8_t msg_id() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return header.msg_id;
    }

    /**
     * @brief Returns the declared payload length.
     */
    CASTLE_NODISCARD uint16_t payload_length() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return header.payload_length;
    }

    /**
     * @brief Reports whether this message has the requested class/ID.
     */
    CASTLE_NODISCARD bool is(uint8_t msg_class_value,
                             uint8_t msg_id_value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (header.msg_class == msg_class_value)
            && (header.msg_id == msg_id_value);
    }

    /**
     * @brief Returns a compact class/ID key.
     */
    CASTLE_NODISCARD uint16_t key() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint16_t>(
            (static_cast<uint16_t>(header.msg_class) << UBX_BITS_PER_BYTE)
            | static_cast<uint16_t>(header.msg_id));
    }
};

/**
 * @brief Streaming Fletcher-8 checksum accumulator.
 *
 * UBX checksums cover CLASS, ID, LEN_LO, LEN_HI, and every payload byte.
 * The parser feeds each byte once as it arrives, avoiding a second pass.
 */
class checksum_accumulator CASTLE_FINAL
{
public:
    /** @brief Constructs an accumulator with both checksum bytes cleared. */
    checksum_accumulator() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Clears the checksum state.
     */
    void reset() CASTLE_NOEXCEPT
    {
        ck_a_ = 0U;
        ck_b_ = 0U;
    }

    /**
     * @brief Adds one protocol byte to the Fletcher-8 state.
     */
    void update(uint8_t value) CASTLE_NOEXCEPT
    {
        ck_a_ = static_cast<uint8_t>(ck_a_ + value);
        ck_b_ = static_cast<uint8_t>(ck_b_ + ck_a_);
    }

    /**
     * @brief Returns the current checksum pair.
     */
    CASTLE_NODISCARD checksum value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return checksum{ck_a_, ck_b_};
    }

private:
    uint8_t ck_a_ = 0U;
    uint8_t ck_b_ = 0U;
};

/**
 * @brief Computes a UBX Fletcher-8 checksum over a byte range.
 */
CASTLE_NODISCARD CASTLE_INLINE checksum compute_checksum(
    castle::container::array_view<CASTLE_CONST uint8_t> data) CASTLE_NOEXCEPT
{
    checksum_accumulator accumulator;
    for (castle::size_type index = 0U; index < data.size(); ++index)
    {
        accumulator.update(data[index]);
    }
    return accumulator.value();
}

/**
 * @brief Computes a UBX Fletcher-8 checksum over class, ID, length and payload.
 */
CASTLE_NODISCARD CASTLE_INLINE checksum compute_frame_checksum(
    uint8_t msg_class,
    uint8_t msg_id,
    uint16_t payload_length,
    castle::container::array_view<CASTLE_CONST uint8_t> payload) CASTLE_NOEXCEPT
{
    checksum_accumulator accumulator;
    accumulator.update(msg_class);
    accumulator.update(msg_id);
    accumulator.update(static_cast<uint8_t>(payload_length & UBX_BYTE_MASK));
    accumulator.update(static_cast<uint8_t>((payload_length >> UBX_BITS_PER_BYTE) & UBX_BYTE_MASK));
    for (castle::size_type index = 0U; index < payload.size(); ++index) // LCOV_EXCL_BR_LINE
    {
        accumulator.update(payload[index]);
    }
    return accumulator.value();
}

/**
 * @brief Returns the complete encoded frame size for a payload.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR castle::size_type frame_size(castle::size_type payload_length) CASTLE_NOEXCEPT
{
    return payload_length + UBX_FRAME_OVERHEAD;
}

/**
 * @brief Encodes one complete UBX frame into a caller-provided buffer.
 *
 * @param msg_class UBX message class.
 * @param msg_id UBX message ID.
 * @param payload Message payload.
 * @param output Destination frame buffer.
 * @param output_capacity Size of the destination buffer.
 * @param[out] bytes_written Number of bytes generated on success, otherwise zero.
 * @return Castle status code.
 *
 * @note This function never allocates and never writes outside `output`.
 */
CASTLE_NODISCARD CASTLE_INLINE castle::status encode_frame(
    uint8_t msg_class,
    uint8_t msg_id,
    castle::container::array_view<CASTLE_CONST uint8_t> payload,
    uint8_t* output,
    castle::size_type output_capacity,
    castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;

    if (payload.size() > static_cast<castle::size_type>(UBX_MAX_PAYLOAD_LEN)) // LCOV_EXCL_BR_LINE
    {
        return castle::status::out_of_range;
    }

    if ((payload.size() > 0U) && (payload.data() == nullptr)) // LCOV_EXCL_BR_LINE
    {
        return castle::status::invalid_argument;
    }

    if (output == nullptr) // LCOV_EXCL_BR_LINE
    {
        return castle::status::invalid_argument;
    }

    CASTLE_CONST castle::size_type required = frame_size(payload.size());
    if (output_capacity < required) // LCOV_EXCL_BR_LINE
    {
        return castle::status::full;
    }

    output[UBX_FRAME_INDEX_SYNC_1] = UBX_SYNC_CHAR_1;
    output[UBX_FRAME_INDEX_SYNC_2] = UBX_SYNC_CHAR_2;
    output[UBX_FRAME_INDEX_CLASS] = msg_class;
    output[UBX_FRAME_INDEX_ID] = msg_id;
    castle::write_le16(&output[UBX_FRAME_INDEX_LENGTH], static_cast<uint16_t>(payload.size()));

    for (castle::size_type index = 0U; index < payload.size(); ++index) // LCOV_EXCL_BR_LINE
    {
        output[UBX_FRAME_INDEX_PAYLOAD + index] = payload[index];
    }

    CASTLE_CONST checksum check = compute_frame_checksum(
        msg_class,
        msg_id,
        static_cast<uint16_t>(payload.size()),
        payload);

    CASTLE_CONST castle::size_type ck_a_index = UBX_FRAME_INDEX_PAYLOAD + payload.size();
    output[ck_a_index] = check.ck_a;
    output[ck_a_index + 1U] = check.ck_b;

    bytes_written = required;
    return castle::status::ok;
}

/**
 * @brief Validates and creates a non-owning view over a complete UBX frame.
 *
 * The view references the supplied frame buffer. No data is copied.
 *
 * @return `true` when the frame is structurally valid and its checksum matches.
 */
CASTLE_NODISCARD CASTLE_INLINE bool decode_frame(
    castle::container::array_view<CASTLE_CONST uint8_t> frame,
    message_view& output) CASTLE_NOEXCEPT
{
    output = message_view{};

    if (frame.size() < UBX_FRAME_OVERHEAD) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    if ((frame[UBX_FRAME_INDEX_SYNC_1] != UBX_SYNC_CHAR_1) || (frame[UBX_FRAME_INDEX_SYNC_2] != UBX_SYNC_CHAR_2)) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    CASTLE_CONST uint16_t payload_length = castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]);
    CASTLE_CONST castle::size_type expected_size = frame_size(payload_length);

    if (frame.size() != expected_size) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    castle::container::array_view<CASTLE_CONST uint8_t> payload(
        payload_length == 0U ? nullptr : &frame[UBX_FRAME_INDEX_PAYLOAD], payload_length); // LCOV_EXCL_BR_LINE

    CASTLE_CONST checksum expected = compute_frame_checksum(
        frame[UBX_FRAME_INDEX_CLASS],
        frame[UBX_FRAME_INDEX_ID],
        payload_length,
        payload);

    CASTLE_CONST castle::size_type ck_a_index = UBX_FRAME_INDEX_PAYLOAD + payload_length;
    CASTLE_CONST castle::size_type ck_b_index = ck_a_index + 1U;

    if ((frame[ck_a_index] != expected.ck_a) || (frame[ck_b_index] != expected.ck_b)) // LCOV_EXCL_BR_LINE
    {
        return false;
    }

    output.header.msg_class = frame[UBX_FRAME_INDEX_CLASS];
    output.header.msg_id = frame[UBX_FRAME_INDEX_ID];
    output.header.payload_length = payload_length;
    output.payload = payload;
    output.check.ck_a = frame[ck_a_index];
    output.check.ck_b = frame[ck_b_index];
    return true;
}

/**
 * @brief Bit position of the 4-bit size code within a UBX CFG key ID.
 */
static CASTLE_CONSTEXPR uint32_t UBX_CFG_KEY_SIZE_CODE_SHIFT = 28U;

/**
 * @brief Mask isolating the 4-bit size code within a UBX CFG key ID.
 */
static CASTLE_CONSTEXPR uint32_t UBX_CFG_KEY_SIZE_CODE_MASK = 0x0FU;

/**
 * @brief Returns the byte count occupied by a known UBX field type.
 *
 * @param key_id UBX configuration key ID.
 * @return Encoded value width in bytes, or 0 for an unknown size code.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR uint8_t value_byte_size(uint32_t key_id) CASTLE_NOEXCEPT
{
    switch ((key_id >> UBX_CFG_KEY_SIZE_CODE_SHIFT) & UBX_CFG_KEY_SIZE_CODE_MASK)
    {
        case 1U: CASTLE_FALL_THROUGH;
        case 2U: return 1U;
        case 3U: return 2U;
        case 4U: return 4U;
        case 5U: return 8U;
        default: return 0U;
    }
}

} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_UBX_HPP
