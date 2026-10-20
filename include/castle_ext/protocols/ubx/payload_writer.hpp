// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file payload_writer.hpp
 * @brief Bounds-checked sequential little-endian writer and zero-copy message encoder.
 *
 * `payload_writer` is the encoding mirror of `payload_reader`: every `write()`
 * stores one field in UBX little-endian order and advances the cursor. It uses
 * the same sticky-error model: the first overflow (or explicit `fail()`)
 * latches a `castle::status`, every later write becomes a no-op, and the
 * encoder checks `status()` once at the end.
 *
 * `encode_message()` drives any message model exposing
 *
 * @code
 * static constexpr uint8_t msg_class;
 * static constexpr uint8_t msg_id;
 * void encode(payload_writer& writer) const noexcept;
 * @endcode
 *
 * The payload is written directly into the payload slot of the caller's frame
 * buffer, then the header and Fletcher-8 checksum are filled in around it, so
 * no intermediate payload copy or scratch buffer is needed.
 *
 * @code
 * castle::protocols::ubx::messages::cfg_rst reset =
 *     castle::protocols::ubx::messages::cfg_rst::cold_start();
 *
 * uint8_t frame[16U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(reset, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_PAYLOAD_WRITER_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_PAYLOAD_WRITER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle/container/array_view.hpp"
#include "castle/container/string.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/bit_cast.hpp"
#include "castle/utility/bytes.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{

/**
 * @brief Sequential, bounds-checked little-endian writer over a caller-provided payload view.
 *
 * Mirrors `payload_reader`: both wrap a non-owning `castle::container::array_view`
 * and walk it with a cursor; the writer never writes outside the view.
 */
class payload_writer CASTLE_FINAL
{
public:
    /**
     * @param payload Destination payload storage. A view with a null data
     * pointer behaves as zero capacity, so every write latches `status::full`.
     */
    explicit payload_writer(castle::container::array_view<uint8_t> payload) CASTLE_NOEXCEPT
        : payload_((payload.data() != nullptr) ? payload : castle::container::array_view<uint8_t>())
    {
    }

    /** @brief Reports whether every write so far fitted and no `fail()` was recorded. */
    CASTLE_NODISCARD bool ok() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return status_ == castle::status::ok;
    }

    /**
     * @brief Returns the latched outcome: `ok`, `full` after an overflow, or the
     * first status passed to `fail()`.
     */
    CASTLE_NODISCARD castle::status status() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return status_;
    }

    /** @brief Returns the number of bytes written so far. */
    CASTLE_NODISCARD castle::size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return position_;
    }

    /** @brief Returns the number of writable bytes left. */
    CASTLE_NODISCARD castle::size_type remaining() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return payload_.size() - position_;
    }

    /**
     * @brief Latches an encoder-specific failure such as an out-of-range field value.
     * @note Only the first failure is kept; later writes become no-ops.
     */
    void fail(castle::status reason) CASTLE_NOEXCEPT
    {
        if (status_ == castle::status::ok)
        {
            status_ = reason;
        }
    }

    /** @brief Writes `count` zero bytes, e.g. for UBX `reserved` fields. */
    void write_zeros(castle::size_type count) CASTLE_NOEXCEPT
    {
        uint8_t* target = advance(count);
        if (target != nullptr)
        {
            for (castle::size_type index = 0U; index < count; ++index)
            {
                target[index] = 0U;
            }
        }
    }

    /** @brief Copies `count` raw bytes; `data` may be `nullptr` only when `count == 0`. */
    void write_bytes(CASTLE_CONST uint8_t* data, castle::size_type count) CASTLE_NOEXCEPT
    {
        uint8_t* target = advance(count);
        if (target != nullptr)
        {
            for (castle::size_type index = 0U; index < count; ++index)
            {
                target[index] = data[index];
            }
        }
    }

    /**
     * @brief Writes the low `width` bytes of `value` in little-endian order.
     * @note Used where the wire width is a runtime property, e.g. CFG key values.
     */
    void write_uint(uint64_t value, castle::size_type width) CASTLE_NOEXCEPT
    {
        uint8_t* target = advance(width);
        if (target != nullptr)
        {
            for (castle::size_type index = 0U; index < width; ++index)
            {
                target[index] = static_cast<uint8_t>(value >> (index * UBX_BITS_PER_BYTE));
            }
        }
    }

    /** @name Type-deduced field writes
     *  Each overload produces exactly `sizeof(value)` bytes.
     *  @{ */
    void write(char value) CASTLE_NOEXCEPT { write_uint(static_cast<uint8_t>(value), 1U); }
    void write(uint8_t value) CASTLE_NOEXCEPT { write_uint(value, 1U); }
    void write(int8_t value) CASTLE_NOEXCEPT { write_uint(static_cast<uint8_t>(value), 1U); }
    void write(uint16_t value) CASTLE_NOEXCEPT { write_uint(value, 2U); }
    void write(int16_t value) CASTLE_NOEXCEPT { write_uint(static_cast<uint16_t>(value), 2U); }
    void write(uint32_t value) CASTLE_NOEXCEPT { write_uint(value, 4U); }
    void write(int32_t value) CASTLE_NOEXCEPT { write_uint(static_cast<uint32_t>(value), 4U); }
    void write(uint64_t value) CASTLE_NOEXCEPT { write_uint(value, 8U); }
    void write(int64_t value) CASTLE_NOEXCEPT { write_uint(static_cast<uint64_t>(value), 8U); }
    void write(float value) CASTLE_NOEXCEPT { write(castle::bit_cast<uint32_t>(value)); }
    void write(double value) CASTLE_NOEXCEPT { write(castle::bit_cast<uint64_t>(value)); }

    /** @brief Writes an enumeration through its underlying integer type. */
    template <typename Enum,
              castle::meta::enable_if_t<castle::meta::is_enum<Enum>::value, int> = 0>
    void write(Enum value) CASTLE_NOEXCEPT
    {
        write(static_cast<castle::meta::underlying_type_t<Enum>>(value));
    }

    /** @brief Writes `N` consecutive elements, e.g. a UBX `U1[8]` field. */
    template <typename T, castle::size_type N>
    void write(CASTLE_CONST castle::container::array<T, N>& values) CASTLE_NOEXCEPT
    {
        for (CASTLE_CONST T& value : values)
        {
            write(value);
        }
    }
    /** @} */

    /** @name Layout verbs
     *  Shared vocabulary with `payload_reader`, so a message's
     *  `layout(Self&, Stream&)` describes its wire format once for both
     *  decoding and encoding.
     *  @{ */

    /** @brief Encodes `value` as the next field (width follows the type). */
    template <typename T>
    void field(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        write(value);
    }

    /** @brief Writes `count` zero bytes for reserved fields. */
    void reserved(castle::size_type count) CASTLE_NOEXCEPT
    {
        write_zeros(count);
    }

    /** @brief Writes a fixed protocol byte such as a message type or version. */
    void constant(uint8_t value) CASTLE_NOEXCEPT
    {
        write(value);
    }

    /**
     * @brief Writes a fixed-width, NUL-padded ASCII field (UBX `CH[Width]`).
     * @note Exactly `Width` bytes are produced; the string's capacity is `Width`.
     */
    template <castle::size_type Width>
    void text(CASTLE_CONST castle::container::string<Width>& value) CASTLE_NOEXCEPT
    {
        for (CASTLE_CONST char character : value)
        {
            write(character);
        }
        write_zeros(Width - value.size());
    }
    /** @} */

private:
    uint8_t* advance(castle::size_type count) CASTLE_NOEXCEPT
    {
        if (status_ != castle::status::ok)
        {
            return nullptr;
        }
        if (count > remaining())
        {
            status_ = castle::status::full;
            return nullptr;
        }

        uint8_t* target = payload_.data() + position_;
        position_ += count;
        return target;
    }

    castle::container::array_view<uint8_t> payload_;
    castle::size_type position_ = 0U;
    castle::status status_ = castle::status::ok;
};

/**
 * @brief Encodes one message model into a complete UBX frame in a caller-provided buffer.
 *
 * @tparam Message Model exposing `msg_class`, `msg_id` and `void encode(payload_writer&) const`.
 * @param message Message to encode.
 * @param output Destination frame buffer.
 * @param output_capacity Size of `output` in bytes.
 * @param[out] bytes_written Encoded frame size on success, otherwise zero.
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
    if (output_capacity < UBX_FRAME_OVERHEAD)
    {
        return castle::status::full;
    }

    castle::size_type payload_capacity = output_capacity - UBX_FRAME_OVERHEAD;
    if (payload_capacity > static_cast<castle::size_type>(UBX_MAX_PAYLOAD_LEN))
    {
        payload_capacity = UBX_MAX_PAYLOAD_LEN;
    }

    payload_writer writer(castle::container::array_view<uint8_t>(&output[UBX_FRAME_INDEX_PAYLOAD], payload_capacity));
    message.encode(writer);
    if (!writer.ok())
    {
        return writer.status();
    }

    CASTLE_CONST uint16_t payload_length = static_cast<uint16_t>(writer.size());
    output[UBX_FRAME_INDEX_SYNC_1] = UBX_SYNC_CHAR_1;
    output[UBX_FRAME_INDEX_SYNC_2] = UBX_SYNC_CHAR_2;
    output[UBX_FRAME_INDEX_CLASS] = Message::msg_class;
    output[UBX_FRAME_INDEX_ID] = Message::msg_id;
    castle::write_le16(&output[UBX_FRAME_INDEX_LENGTH], payload_length);

    CASTLE_CONST checksum check = compute_frame_checksum(
        Message::msg_class,
        Message::msg_id,
        payload_length,
        castle::container::array_view<CASTLE_CONST uint8_t>(&output[UBX_FRAME_INDEX_PAYLOAD], payload_length));

    CASTLE_CONST castle::size_type ck_a_index = UBX_FRAME_INDEX_PAYLOAD + payload_length;
    output[ck_a_index] = check.ck_a;
    output[ck_a_index + 1U] = check.ck_b;

    bytes_written = frame_size(payload_length);
    return castle::status::ok;
}

/**
 * @brief Encodes the zero-payload poll request of `Message` (e.g. UBX-MON-VER, UBX-NAV-PVT).
 *
 * @tparam Message Model exposing `msg_class` and `msg_id`.
 * @return Forwards `encode_frame()`'s result.
 */
template <typename Message>
CASTLE_NODISCARD CASTLE_INLINE castle::status encode_poll(
    uint8_t* output,
    castle::size_type output_capacity,
    castle::size_type& bytes_written) CASTLE_NOEXCEPT
{
    return encode_frame(
        Message::msg_class,
        Message::msg_id,
        castle::container::array_view<CASTLE_CONST uint8_t>(),
        output,
        output_capacity,
        bytes_written);
}

} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_PAYLOAD_WRITER_HPP
