// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file payload_reader.hpp
 * @brief Bounds-checked sequential cursor over a validated UBX payload.
 *
 * Message decoders need to walk a payload field-by-field without hand
 * computing byte offsets (error-prone) or reinterpret_cast-ing the buffer
 * into a packed struct (unsafe: alignment and endianness are not portable).
 * `payload_reader` wraps `protocols::ubx::read_le16/32/64(s)` with a cursor
 * so every `read_*()` call both decodes the next field and advances past it.
 *
 * The reader uses a sticky-error model: once a read runs past the end of the
 * payload, it starts returning zero and every subsequent read keeps failing.
 * Decoders therefore only need one trailing `ok()` check instead of guarding
 * every individual field access.
 *
 * @code
 * castle_ext::protocols::ubx::payload_reader reader(message.payload);
 * uint32_t i_tow = reader.read_u32();
 * uint16_t year = reader.read_u16();
 * if (!reader.ok()) { return false; } // payload shorter than expected
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_PAYLOAD_READER_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_PAYLOAD_READER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array_view.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"

#include <stdint.h>

namespace castle_ext
{
namespace protocols
{
namespace ubx
{

/**
 * @brief Sequential, bounds-checked little-endian reader over a UBX payload view.
 */
class payload_reader CASTLE_FINAL
{
public:
    explicit payload_reader(
        castle::container::array_view<CASTLE_CONST uint8_t> payload) CASTLE_NOEXCEPT
        : payload_(payload)
    {
    }

    /** @brief Reports whether every read so far stayed within the payload. */
    CASTLE_NODISCARD bool ok() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return ok_;
    }

    /** @brief Returns the current cursor offset in bytes. */
    CASTLE_NODISCARD castle::size_type position() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return position_;
    }

    /** @brief Returns the number of unread bytes remaining. */
    CASTLE_NODISCARD castle::size_type remaining() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (position_ <= payload_.size()) ? (payload_.size() - position_) : 0U; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Advances the cursor by `count` bytes without inspecting them.
     * @note The return value is informational only; a failed skip already
     * latches `ok()` to `false`, which every decoder checks once at the end.
     */
    bool skip(castle::size_type count) CASTLE_NOEXCEPT
    {
        return advance(count) != nullptr;
    }

    /** @brief Returns a pointer to `count` raw bytes, e.g. for fixed-size ASCII fields. */
    CASTLE_NODISCARD CASTLE_CONST uint8_t* read_bytes(castle::size_type count) CASTLE_NOEXCEPT
    {
        return advance(count);
    }

    CASTLE_NODISCARD uint8_t read_u8() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(1U);
        return (data != nullptr) ? data[0U] : 0U; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD int8_t read_i8() CASTLE_NOEXCEPT
    {
        return static_cast<int8_t>(read_u8());
    }

    CASTLE_NODISCARD uint16_t read_u16() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(2U);
        return (data != nullptr) ? read_le16(data) : 0U; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD int16_t read_i16() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(2U);
        return (data != nullptr) ? read_le16s(data) : 0; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD uint32_t read_u32() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(4U);
        return (data != nullptr) ? read_le32(data) : 0U; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD int32_t read_i32() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(4U);
        return (data != nullptr) ? read_le32s(data) : 0; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD uint64_t read_u64() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(8U);
        return (data != nullptr) ? read_le64(data) : 0U; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD int64_t read_i64() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(8U);
        return (data != nullptr) ? read_le64s(data) : 0; // LCOV_EXCL_BR_LINE
    }

private:
    CASTLE_CONST uint8_t* advance(castle::size_type count) CASTLE_NOEXCEPT
    {
        if (!ok_ || (count > remaining())) // LCOV_EXCL_BR_LINE
        {
            ok_ = false;
            return nullptr;
        }

        CASTLE_CONST uint8_t* data = &payload_[position_];
        position_ += count;
        return data;
    }

    castle::container::array_view<CASTLE_CONST uint8_t> payload_;
    castle::size_type position_ = 0U;
    bool ok_ = true;
};

} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_PAYLOAD_READER_HPP
