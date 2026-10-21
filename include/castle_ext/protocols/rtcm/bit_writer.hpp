// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bit_writer.hpp
 * @brief Bounds-checked big-endian bit writer and zero-copy RTCM message encoder.
 *
 * RTCM 3 payloads are bit streams: data fields (DF) are 1..64 bits wide and
 * are packed most-significant bit first with no alignment between fields. The
 * `bit_writer` appends such fields to a caller-provided buffer.
 *
 * It uses the same sticky-error model as the UBX `payload_writer`: the first
 * overflow, out-of-range value or explicit `fail()` latches a `castle::status`;
 * every later write becomes a no-op, and the encoder checks `status()` once at
 * the end. Unused bits of the last byte are zero, which is the padding RTCM
 * requires.
 *
 * `encode_message()` drives any message model exposing
 *
 * @code
 * void encode(bit_writer& writer) const noexcept;
 * @endcode
 *
 * The payload is written directly into the payload slot of the caller's frame
 * buffer, then the header and CRC-24Q are filled in around it, so no
 * intermediate copy or scratch buffer is needed.
 *
 * @code
 * castle::protocols::rtcm::messages::station_arp station{};
 * station.station_id = 1U;
 * station.set_position_m(4000000.0, 3000000.0, 5000000.0);
 *
 * uint8_t frame[32U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::rtcm::encode_message(station, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_BIT_WRITER_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_BIT_WRITER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array_view.hpp"
#include "castle/container/string.hpp"
#include "castle/error/status.hpp"

#include "castle_ext/protocols/rtcm/rtcm.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace rtcm
{

/**
 * @brief Sequential, bounds-checked MSB-first bit writer over a caller-provided payload view.
 */
class bit_writer CASTLE_FINAL
{
public:
    /**
     * @param payload Destination storage. A view with a null data pointer behaves
     * as zero capacity, so the first write latches `status::full`.
     */
    explicit bit_writer(castle::container::array_view<uint8_t> payload) CASTLE_NOEXCEPT
        : payload_((payload.data() != nullptr) ? payload : castle::container::array_view<uint8_t>())
    {
    }

    /** @brief Reports whether every write so far fitted and no `fail()` was recorded. */
    CASTLE_NODISCARD bool ok() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return status_ == castle::status::ok;
    }

    /** @brief Returns the latched outcome: `ok`, `full`, `out_of_range`, or the first `fail()` reason. */
    CASTLE_NODISCARD castle::status status() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return status_;
    }

    /** @brief Returns the number of bits written so far. */
    CASTLE_NODISCARD castle::size_type bit_size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return position_;
    }

    /** @brief Returns the payload size in bytes, i.e. the bits written rounded up to a whole byte. */
    CASTLE_NODISCARD castle::size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (position_ + 7U) / 8U;
    }

    /** @brief Returns the number of bits that can still be written. */
    CASTLE_NODISCARD castle::size_type remaining_bits() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (payload_.size() * 8U) - position_;
    }

    /**
     * @brief Latches an encoder-specific failure such as an invalid message number.
     * @note Only the first failure is kept; later writes become no-ops.
     */
    void fail(castle::status reason) CASTLE_NOEXCEPT
    {
        if (status_ == castle::status::ok)
        {
            status_ = reason;
        }
    }

    /**
     * @brief Appends the low `width` bits of `value`, most significant bit first.
     * @param value Raw bit pattern; bits above `width` must be zero.
     * @param width Field width, 0..64. A wider value latches `out_of_range`.
     */
    void write_bits(uint64_t value, castle::size_type width) CASTLE_NOEXCEPT
    {
        if (status_ != castle::status::ok)
        {
            return;
        }
        if ((width > 64U) || ((width < 64U) && ((value >> width) != 0U)))
        {
            status_ = castle::status::out_of_range;
            return;
        }
        if (width > remaining_bits())
        {
            status_ = castle::status::full;
            return;
        }

        while (width > 0U)
        {
            CASTLE_CONST castle::size_type offset = position_ & 7U;
            CASTLE_CONST castle::size_type room = 8U - offset;
            CASTLE_CONST castle::size_type take = (width < room) ? width : room;
            CASTLE_CONST uint8_t chunk = static_cast<uint8_t>((value >> (width - take)) & ((1U << take) - 1U));

            uint8_t& target = payload_[position_ >> 3U];
            if (offset == 0U)
            {
                target = 0U; // A fresh byte starts zeroed, so trailing pad bits are always zero.
            }
            target = static_cast<uint8_t>(target | (chunk << (room - take)));

            position_ += take;
            width -= take;
        }
    }

    /** @name Data fields
     *  Each overload checks that the value fits `bits` and latches `out_of_range` otherwise.
     *  @{ */

    /** @brief Writes an unsigned data field (RTCM `uintN`). */
    template <typename T,
              castle::meta::enable_if_t<castle::meta::is_integral<T>::value
                                            && castle::meta::is_unsigned<T>::value
                                            && !castle::meta::is_same<T, bool>::value,
                                        int> = 0>
    void field(T value, castle::size_type bits) CASTLE_NOEXCEPT
    {
        write_bits(static_cast<uint64_t>(value), bits);
    }

    /** @brief Writes a two's complement data field (RTCM `intN`). */
    template <typename T,
              castle::meta::enable_if_t<castle::meta::is_integral<T>::value
                                            && castle::meta::is_signed<T>::value,
                                        int> = 0>
    void field(T value, castle::size_type bits) CASTLE_NOEXCEPT
    {
        CASTLE_CONST int64_t signed_value = static_cast<int64_t>(value);
        if ((bits == 0U) || (bits > 64U))
        {
            fail(castle::status::out_of_range);
            return;
        }
        if (bits < 64U)
        {
            CASTLE_CONST int64_t limit = static_cast<int64_t>(1) << (bits - 1U);
            if ((signed_value < -limit) || (signed_value >= limit))
            {
                fail(castle::status::out_of_range);
                return;
            }
        }
        CASTLE_CONST uint64_t mask = (bits == 64U) ? ~static_cast<uint64_t>(0) : ((static_cast<uint64_t>(1) << bits) - 1U);
        write_bits(static_cast<uint64_t>(signed_value) & mask, bits);
    }

    /** @brief Writes a one-bit flag. */
    void field(bool value) CASTLE_NOEXCEPT
    {
        write_bits(value ? 1U : 0U, 1U);
    }
    /** @} */

    /** @brief Writes `bits` reserved zero bits. */
    void reserved(castle::size_type bits) CASTLE_NOEXCEPT
    {
        while (bits > 64U)
        {
            write_bits(0U, 64U);
            bits -= 64U;
        }
        write_bits(0U, bits);
    }

    /**
     * @brief Writes an RTCM character string: an 8-bit character count followed by the 8-bit characters.
     * @note Matches DF029/DF030 (descriptor counter + descriptor) and their siblings.
     */
    template <castle::size_type N>
    void text(CASTLE_CONST castle::container::string<N>& value) CASTLE_NOEXCEPT
    {
        static_assert(N <= 255U, "an RTCM string is counted by an 8-bit field");
        field(static_cast<uint8_t>(value.size()), 8U);
        for (CASTLE_CONST char character : value)
        {
            field(static_cast<uint8_t>(character), 8U);
        }
    }

private:
    castle::container::array_view<uint8_t> payload_;
    castle::size_type position_ = 0U;
    castle::status status_ = castle::status::ok;
};

/**
 * @brief Encodes one message model into a complete RTCM 3 frame in a caller-provided buffer.
 *
 * @tparam Message Model exposing `void encode(bit_writer&) const`.
 * @param message Message to encode.
 * @param output Destination frame buffer.
 * @param output_capacity Size of `output` in bytes.
 * @param[out] bytes_written Frame size on success, otherwise zero.
 * @return `invalid_argument` for a null `output`; `full` when the frame does not
 * fit; otherwise the first failure latched by `Message::encode()` or `ok`.
 *
 * @note The payload is encoded in place; no scratch buffer is used.
 */
template <typename Message>
CASTLE_NODISCARD CASTLE_INLINE castle::status encode_message(
    CASTLE_CONST Message& message,
    uint8_t* output,
    castle::size_type output_capacity,
    castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    bytes_written = 0U;

    if (output == nullptr)
    {
        return castle::status::invalid_argument;
    }
    if (output_capacity < RTCM_FRAME_OVERHEAD)
    {
        return castle::status::full;
    }

    castle::size_type payload_capacity = output_capacity - RTCM_FRAME_OVERHEAD;
    if (payload_capacity > RTCM_MAX_PAYLOAD_LEN)
    {
        payload_capacity = RTCM_MAX_PAYLOAD_LEN;
    }

    bit_writer writer(castle::container::array_view<uint8_t>(&output[RTCM_FRAME_INDEX_PAYLOAD], payload_capacity));
    message.encode(writer);
    if (!writer.ok())
    {
        return writer.status();
    }

    bytes_written = detail::finish_frame(output, writer.size());
    return castle::status::ok;
}

} // namespace rtcm
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_RTCM_BIT_WRITER_HPP
