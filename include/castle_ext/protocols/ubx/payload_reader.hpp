// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file payload_reader.hpp
 * @brief Bounds-checked sequential cursor over a validated UBX payload.
 *
 * Message decoders need to walk a payload field-by-field without hand
 * computing byte offsets (error-prone) or reinterpret_cast-ing the buffer
 * into a packed struct (unsafe: alignment and endianness are not portable).
 * `payload_reader` wraps `castle::read_le16/32/64` and their signed variants with a cursor
 * so every `read_*()` call both decodes the next field and advances past it.
 *
 * The reader uses a sticky-error model: once a read runs past the end of the
 * payload, it starts returning zero and every subsequent read keeps failing.
 * Decoders therefore only need one trailing `ok()` check instead of guarding
 * every individual field access.
 *
 * Besides the width-named `read_u8()` ... `read_r8()` helpers, the type-deduced
 * `read(T&)` overload set reads exactly `sizeof(T)` bytes into a field, so a
 * decoder's wire width always follows the declared field type. Enumerations
 * are read through their underlying type and `castle::container::array`
 * fields are filled element by element.
 *
 * @code
 * castle::protocols::ubx::payload_reader reader(message.payload);
 * uint32_t i_tow = reader.read_u32();
 * uint16_t year = 0U;
 * reader.read(year);
 * if (!reader.ok()) { return false; } // payload shorter than expected
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_PAYLOAD_READER_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_PAYLOAD_READER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle/container/array_view.hpp"
#include "castle/container/string.hpp"
#include "castle/utility/bit_cast.hpp"

#include "castle_ext/protocols/ubx/ubx.hpp"

#include <stdint.h>

namespace castle
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
        : payload_(payload.data() != nullptr ? payload : castle::container::array_view<CASTLE_CONST uint8_t>())
        , position_(0U)
        , ok_(payload.data() != nullptr)
    {
    }

    /** @brief Reports whether every read so far stayed within the payload. */
    CASTLE_NODISCARD bool ok() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return ok_;
    }

    /**
     * @brief Latches a decoder-specific failure, e.g. an unexpected constant byte.
     * @note Every later read then returns zero, exactly as after an overrun.
     */
    void fail() CASTLE_NOEXCEPT
    {
        ok_ = false;
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
        return (data != nullptr) ? castle::read_le16(data) : 0U; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD int16_t read_i16() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(2U);
        return (data != nullptr) ? castle::read_le16s(data) : 0; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD uint32_t read_u32() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(4U);
        return (data != nullptr) ? castle::read_le32(data) : 0U; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD int32_t read_i32() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(4U);
        return (data != nullptr) ? castle::read_le32s(data) : 0; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD uint64_t read_u64() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(8U);
        return (data != nullptr) ? castle::read_le64(data) : 0U; // LCOV_EXCL_BR_LINE
    }

    CASTLE_NODISCARD int64_t read_i64() CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t* data = advance(8U);
        return (data != nullptr) ? castle::read_le64s(data) : 0; // LCOV_EXCL_BR_LINE
    }

    /** @brief Reads an IEEE-754 single-precision value (UBX type `R4`). */
    CASTLE_NODISCARD float read_r4() CASTLE_NOEXCEPT
    {
        return castle::bit_cast<float>(read_u32());
    }

    /** @brief Reads an IEEE-754 double-precision value (UBX type `R8`). */
    CASTLE_NODISCARD double read_r8() CASTLE_NOEXCEPT
    {
        return castle::bit_cast<double>(read_u64());
    }

    /** @name Type-deduced field reads
     *  Each overload consumes exactly `sizeof(value)` bytes.
     */
    void read(char& value) CASTLE_NOEXCEPT { value = static_cast<char>(read_u8()); }
    void read(uint8_t& value) CASTLE_NOEXCEPT { value = read_u8(); }
    void read(int8_t& value) CASTLE_NOEXCEPT { value = read_i8(); }
    void read(uint16_t& value) CASTLE_NOEXCEPT { value = read_u16(); }
    void read(int16_t& value) CASTLE_NOEXCEPT { value = read_i16(); }
    void read(uint32_t& value) CASTLE_NOEXCEPT { value = read_u32(); }
    void read(int32_t& value) CASTLE_NOEXCEPT { value = read_i32(); }
    void read(uint64_t& value) CASTLE_NOEXCEPT { value = read_u64(); }
    void read(int64_t& value) CASTLE_NOEXCEPT { value = read_i64(); }
    void read(float& value) CASTLE_NOEXCEPT { value = read_r4(); }
    void read(double& value) CASTLE_NOEXCEPT { value = read_r8(); }

    /** @brief Reads an enumeration through its underlying integer type. */
    template <typename Enum,
              castle::meta::enable_if_t<castle::meta::is_enum<Enum>::value, int> = 0>
    void read(Enum& value) CASTLE_NOEXCEPT
    {
        castle::meta::underlying_type_t<Enum> raw{};
        read(raw);
        value = static_cast<Enum>(raw);
    }

    /** @brief Reads `N` consecutive elements, e.g. a UBX `U2[6]` or `CH[30]` field. */
    template <typename T, castle::size_type N>
    void read(castle::container::array<T, N>& values) CASTLE_NOEXCEPT
    {
        for (T& value : values)
        {
            read(value);
        }
    }

    /** @name Layout verbs
     *  Shared vocabulary with `payload_writer`, so a message's
     *  `layout(Self&, Stream&)` describes its wire format once for both
     *  decoding and encoding.
     *  @{ */

    /** @brief Decodes the next field into `value` (width follows the type). */
    template <typename T>
    void field(T& value) CASTLE_NOEXCEPT
    {
        read(value);
    }

    /** @brief Skips `count` reserved bytes. */
    void reserved(castle::size_type count) CASTLE_NOEXCEPT
    {
        static_cast<void>(skip(count));
    }

    /** @brief Consumes one byte and latches a failure unless it equals `expected`. */
    void constant(uint8_t expected) CASTLE_NOEXCEPT
    {
        if (read_u8() != expected)
        {
            fail();
        }
    }

    /**
     * @brief Reads a fixed-width, NUL-padded ASCII field (UBX `CH[Width]`).
     *
     * Exactly `Width` bytes are consumed; characters up to the first NUL are
     * stored in `text`.
     */
    template <castle::size_type Width>
    void text(castle::container::string<Width>& value) CASTLE_NOEXCEPT
    {
        value.clear();
        CASTLE_CONST uint8_t* data = advance(Width);
        if (data == nullptr)
        {
            return;
        }
        for (castle::size_type index = 0U; (index < Width) && (data[index] != 0U); ++index)
        {
            static_cast<void>(value.push_back(static_cast<char>(data[index])));
        }
    }
    /** @} */

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
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_PAYLOAD_READER_HPP
