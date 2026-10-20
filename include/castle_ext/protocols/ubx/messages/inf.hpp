// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file inf.hpp
 * @brief UBX-INF-ERROR/WARNING/NOTICE/TEST/DEBUG (0x04 0x00..0x04): firmware text output.
 *
 * Output messages whose whole payload (`[0..n]` bytes, possibly empty) is an
 * ASCII string. The string is not NUL-terminated on the wire; up to `MaxText`
 * characters are stored and decoding stops early at a NUL byte if one is
 * present. The severity is carried in the message ID.
 *
 *  - `basic_inf<MaxText, Id>` models one severity, so a decoder registry can
 *    subscribe to e.g. errors only (`inf_error<>`, `inf_warning<>`, ...);
 *  - `inf<MaxText>` matches all five IDs and records which one arrived.
 *
 * Both share one text decoder.
 *
 * @code
 * castle::protocols::ubx::messages::inf_error<> error{};
 * if (castle::protocols::ubx::messages::inf_error<>::decode(raw, error)) { log(error.text.c_str()); }
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_INF_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_INF_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-INF severity; the enumerator value is the message ID. */
enum class inf_subtype : uint8_t
{
    error = UBX_ID_INF_ERROR,
    warning = UBX_ID_INF_WARNING,
    notice = UBX_ID_INF_NOTICE,
    test = UBX_ID_INF_TEST,
    debug = UBX_ID_INF_DEBUG
};

namespace detail
{

/**
 * @brief Copies up to `MaxText` payload characters into `text`, stopping at the first NUL.
 * @note Reads `raw.payload` directly, so an empty payload (null view) yields an empty string.
 */
template <castle::size_type MaxText>
CASTLE_INLINE void read_inf_text(CASTLE_CONST message_view& raw, castle::container::string<MaxText>& text) CASTLE_NOEXCEPT
{
    text.clear();
    for (castle::size_type index = 0U; (index < raw.payload.size()) && !text.full(); ++index)
    {
        CASTLE_CONST uint8_t character = raw.payload[index];
        if (character == 0U)
        {
            return;
        }
        static_cast<void>(text.push_back(static_cast<char>(character)));
    }
}

} // namespace detail

/** @brief One UBX-INF severity, selected by `MsgId`. */
template <castle::size_type MaxText, uint8_t MsgId>
struct basic_inf
{
    static_assert(MsgId <= UBX_ID_INF_DEBUG, "UBX-INF message IDs are 0x00..0x04");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_INF;
    static CASTLE_CONSTEXPR uint8_t msg_id = MsgId;
    static CASTLE_CONSTEXPR inf_subtype subtype = static_cast<inf_subtype>(MsgId);

    castle::container::string<MaxText> text{}; ///< ASCII text (wire bytes before any NUL).

    /** @brief Reports whether `raw` carries this severity's class/ID. */
    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the text (any payload length, including zero, is valid).
     * @return `false` for another class/ID; `out` is then value-initialised.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, basic_inf& out) CASTLE_NOEXCEPT
    {
        out = basic_inf{};
        if (!matches(raw))
        {
            return false;
        }
        detail::read_inf_text(raw, out.text);
        return true;
    }

    /** @brief Writes the stored characters without a NUL terminator. */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (CASTLE_CONST char character : text)
        {
            writer.write(character);
        }
    }
};

/** @brief UBX-INF-ERROR (0x04 0x00). */
template <castle::size_type MaxText = 256U>
using inf_error = basic_inf<MaxText, UBX_ID_INF_ERROR>;

/** @brief UBX-INF-WARNING (0x04 0x01). */
template <castle::size_type MaxText = 256U>
using inf_warning = basic_inf<MaxText, UBX_ID_INF_WARNING>;

/** @brief UBX-INF-NOTICE (0x04 0x02). */
template <castle::size_type MaxText = 256U>
using inf_notice = basic_inf<MaxText, UBX_ID_INF_NOTICE>;

/** @brief UBX-INF-TEST (0x04 0x03). */
template <castle::size_type MaxText = 256U>
using inf_test = basic_inf<MaxText, UBX_ID_INF_TEST>;

/** @brief UBX-INF-DEBUG (0x04 0x04). */
template <castle::size_type MaxText = 256U>
using inf_debug = basic_inf<MaxText, UBX_ID_INF_DEBUG>;

/** @brief Any UBX-INF severity; `message_id` / `subtype` record which one arrived. */
template <castle::size_type MaxText = 256U>
struct inf
{
    static_assert(UBX_ID_INF_ERROR == 0U, "matches() relies on the INF IDs starting at zero");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_INF;

    uint8_t message_id = UBX_ID_INF_ERROR;      ///< Received message ID.
    inf_subtype subtype = inf_subtype::error;   ///< Severity, same value as `message_id`.
    castle::container::string<MaxText> text{};  ///< ASCII text (wire bytes before any NUL).

    /** @brief Reports whether `raw` is any of the five UBX-INF messages. */
    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return (raw.msg_class() == UBX_CLASS_INF) && (raw.msg_id() <= UBX_ID_INF_DEBUG);
    }

    /**
     * @brief Decodes the severity and text (any payload length, including zero, is valid).
     * @return `false` for another class/ID; `out` is then value-initialised.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, inf& out) CASTLE_NOEXCEPT
    {
        out = inf{};
        if (!matches(raw))
        {
            return false;
        }
        out.message_id = raw.msg_id();
        out.subtype = static_cast<inf_subtype>(raw.msg_id());
        detail::read_inf_text(raw, out.text);
        return true;
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_INF_HPP
