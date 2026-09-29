// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file crc.hpp
 * @brief CAN Classical CRC-15 and ISO CAN FD CRC-17/CRC-21 calculation.
 *
 * CAN checksums are bit-oriented and include protocol fields that are not byte
 * aligned. CAN FD additionally feeds dynamic stuff bits and the stuff-count
 * field into its CRC.
 */
#ifndef CASTLE_EXT_PROTOCOLS_CAN_CRC_HPP
#define CASTLE_EXT_PROTOCOLS_CAN_CRC_HPP

#include "castle/core/compiler.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/protocols/can/frame.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace can
{

static CASTLE_CONSTEXPR uint8_t CAN_CLASSICAL_CRC_WIDTH = 15U;
static CASTLE_CONSTEXPR uint8_t CAN_FD_CRC17_WIDTH = 17U;
static CASTLE_CONSTEXPR uint8_t CAN_FD_CRC21_WIDTH = 21U;
static CASTLE_CONSTEXPR uint32_t CAN_CLASSICAL_CRC_POLYNOMIAL = 0x4599U;
static CASTLE_CONSTEXPR uint32_t CAN_FD_CRC17_POLYNOMIAL = 0x1685BU;
static CASTLE_CONSTEXPR uint32_t CAN_FD_CRC21_POLYNOMIAL = 0x102899U;

/** @brief Computed CAN frame-check-sequence value and its bit width. */
struct crc_result
{
    uint32_t value = 0U;
    uint8_t width = 0U;
};

namespace detail
{

class bit_crc
{
public:
    bit_crc(uint8_t width, uint32_t polynomial, uint32_t initial) CASTLE_NOEXCEPT
        : width_(width), polynomial_(polynomial), state_(initial),
          mask_((static_cast<uint32_t>(1U) << width) - 1U),
          top_bit_(static_cast<uint32_t>(1U) << (width - 1U))
    {
    }

    void update_bit(bool bit) CASTLE_NOEXCEPT
    {
        const bool feedback = ((state_ & top_bit_) != 0U) != bit;
        state_ = static_cast<uint32_t>((state_ << 1U) & mask_);
        if (feedback)
        {
            state_ ^= polynomial_;
        }
    }

    uint32_t value() CASTLE_CONST CASTLE_NOEXCEPT { return state_; }

private:
    uint8_t width_;
    uint32_t polynomial_;
    uint32_t state_;
    uint32_t mask_;
    uint32_t top_bit_;
};

/** Emits CAN FD's dynamically stuffed bit stream while updating its CRC. */
class fd_bit_stream
{
public:
    explicit fd_bit_stream(bit_crc& crc) CASTLE_NOEXCEPT
        : crc_(crc), run_length_(0U), stuff_count_(0U), last_bit_(false)
    {
    }

    void append(bool bit) CASTLE_NOEXCEPT
    {
        crc_.update_bit(bit);
        if (run_length_ == 0U || bit != last_bit_)
        {
            last_bit_ = bit;
            run_length_ = 1U;
        }
        else
        {
            ++run_length_;
        }

        if (run_length_ == 5U)
        {
            const bool stuff_bit = !last_bit_;
            crc_.update_bit(stuff_bit);
            ++stuff_count_;
            last_bit_ = stuff_bit;
            run_length_ = 1U;
        }
    }

    void append_field(uint32_t value, uint8_t bit_count) CASTLE_NOEXCEPT
    {
        for (uint8_t bit = bit_count; bit > 0U; --bit)
        {
            append(((value >> (bit - 1U)) & 1U) != 0U);
        }
    }

    uint8_t stuff_count_mod8() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>(stuff_count_ & 0x07U);
    }

private:
    bit_crc& crc_;
    uint8_t run_length_;
    uint32_t stuff_count_;
    bool last_bit_;
};

inline void append_classical_field(bit_crc& crc, uint32_t value, uint8_t bit_count) CASTLE_NOEXCEPT
{
    for (uint8_t bit = bit_count; bit > 0U; --bit)
    {
        crc.update_bit(((value >> (bit - 1U)) & 1U) != 0U);
    }
}

inline void append_identifier_and_control(
    const frame& value,
    bool fd,
    bit_crc& classical_crc,
    fd_bit_stream* fd_stream) CASTLE_NOEXCEPT
{
    // SOF is dominant. CRC input is the destuffed sequence for Classical CAN
    // and the dynamically stuffed stream for ISO CAN FD.
    if (fd)
    {
        fd_stream->append(false);
    }
    else
    {
        classical_crc.update_bit(false);
    }

#define CASTLE_CAN_APPEND_FIELD(field_value, bit_count) \
    do { \
        if (fd) { fd_stream->append_field((field_value), (bit_count)); } \
        else { append_classical_field(classical_crc, (field_value), (bit_count)); } \
    } while (false)

    const bool extended = value.id_format == identifier_format::extended;
    if (!extended)
    {
        CASTLE_CAN_APPEND_FIELD(value.identifier, 11U);
        // RTR in Classical CAN; RRS (dominant) in CAN FD.
        CASTLE_CAN_APPEND_FIELD((!fd && value.kind == frame_kind::remote) ? 1U : 0U, 1U); // LCOV_EXCL_BR_LINE
        CASTLE_CAN_APPEND_FIELD(0U, 1U); // IDE = standard identifier
        if (!fd)
        {
            CASTLE_CAN_APPEND_FIELD(0U, 1U); // r0
        }
    }
    else
    {
        // LCOV_EXCL_START
        CASTLE_CAN_APPEND_FIELD((value.identifier >> 18U) & 0x7FFU, 11U);
        CASTLE_CAN_APPEND_FIELD(1U, 1U); // SRR
        CASTLE_CAN_APPEND_FIELD(1U, 1U); // IDE = extended identifier
        CASTLE_CAN_APPEND_FIELD(value.identifier & 0x3FFFFU, 18U);
        CASTLE_CAN_APPEND_FIELD((!fd && value.kind == frame_kind::remote) ? 1U : 0U, 1U);
        if (!fd)
        {
            CASTLE_CAN_APPEND_FIELD(0U, 2U); // r1, r0
        }
        // LCOV_EXCL_STOP
    }

    // LCOV_EXCL_START
    if (fd)
    {
        CASTLE_CAN_APPEND_FIELD(1U, 1U); // FDF
        CASTLE_CAN_APPEND_FIELD(0U, 1U); // reserved
        CASTLE_CAN_APPEND_FIELD(value.bit_rate_switch ? 1U : 0U, 1U);
        CASTLE_CAN_APPEND_FIELD(value.error_state_indicator ? 1U : 0U, 1U);
    }
    // LCOV_EXCL_STOP

    CASTLE_CAN_APPEND_FIELD(value.data_length_code(), 4U);
#undef CASTLE_CAN_APPEND_FIELD
}

inline void append_payload(const frame& value, bit_crc& crc, fd_bit_stream* fd_stream) CASTLE_NOEXCEPT
{
    if (value.kind != frame_kind::data)
    {
        return; // Remote frames have a DLC but no data field.
    }
    for (castle::size_type index = 0U; index < value.data_length; ++index)
    {
        for (uint8_t bit = 8U; bit > 0U; --bit)
        {
            const bool data_bit = ((value.data[index] >> (bit - 1U)) & 1U) != 0U;
            if (fd_stream != nullptr)
            {
                fd_stream->append(data_bit);
            }
            else
            {
                crc.update_bit(data_bit);
            }
        }
    }
}

} // namespace detail

/**
 * @brief Calculates the frame check sequence for a valid data or remote frame.
 * @param value Valid frame value.
 * @param output Receives the checksum value and width on success.
 * @return `ok` on success; `invalid_argument` for malformed frames and for
 * error/overload signaling events, which do not carry an application FCS.
 *
 * Classical CAN CRC-15 covers SOF through the data field (or DLC for remote
 * frames), excluding dynamic stuff bits. ISO CAN FD uses CRC-17 for 0..16 byte
 * payloads and CRC-21 for 20..64 byte payloads; its dynamic stuff bits and
 * Gray-coded stuff-count/parity field are included in the CRC input.
 */
inline castle::status calculate_crc(const frame& value, crc_result& output) CASTLE_NOEXCEPT
{
    output = crc_result{};
    if (!value.valid() || value.kind == frame_kind::error || value.kind == frame_kind::overload)
    {
        return castle::status::invalid_argument;
    }

    if (value.format == frame_format::classical)
    {
        detail::bit_crc crc(CAN_CLASSICAL_CRC_WIDTH, CAN_CLASSICAL_CRC_POLYNOMIAL, 0U);
        detail::append_identifier_and_control(value, false, crc, nullptr);
        detail::append_payload(value, crc, nullptr);
        output.value = crc.value();
        output.width = CAN_CLASSICAL_CRC_WIDTH;
        return castle::status::ok;
    }

    const uint8_t width = value.data_length <= 16U ? CAN_FD_CRC17_WIDTH : CAN_FD_CRC21_WIDTH;
    const uint32_t polynomial = value.data_length <= 16U
        ? CAN_FD_CRC17_POLYNOMIAL : CAN_FD_CRC21_POLYNOMIAL;
    detail::bit_crc crc(width, polynomial, static_cast<uint32_t>(1U) << (width - 1U));
    detail::fd_bit_stream stream(crc);
    detail::append_identifier_and_control(value, true, crc, &stream);
    detail::append_payload(value, crc, &stream);

    // ISO CAN FD encodes the number of dynamic stuff bits modulo 8 as Gray
    // code, followed by even-parity bit. The fixed stuff bits in the CRC field
    // are not part of the CRC input.
    const uint8_t stuff_count = stream.stuff_count_mod8();
    const uint8_t gray = static_cast<uint8_t>(stuff_count ^ (stuff_count >> 1U));
    const bool parity = ((gray & 0x04U) != 0U)
        ^ ((gray & 0x02U) != 0U)
        ^ ((gray & 0x01U) != 0U);
    detail::append_classical_field(crc, gray, 3U);
    crc.update_bit(parity);

    output.value = crc.value();
    output.width = width;
    return castle::status::ok;
}

} // namespace can
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_CAN_CRC_HPP
