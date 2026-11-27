// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file fixed_message.hpp
 * @brief CRTP base that derives `matches()`, `decode()` and `encode()` from one wire layout.
 *
 * Most UBX messages have a fixed payload length. Writing their field order
 * twice (once in `decode`, once in `encode`) is the classic source of
 * reader/writer drift. Instead, a message describes its payload once:
 *
 * @code
 * struct nav_eoe : detail::fixed_message<nav_eoe, UBX_CLASS_NAV, UBX_ID_NAV_EOE, 4U>
 * {
 *     uint32_t i_tow = 0U;
 *
 *     template <typename Self, typename Stream>
 *     static void layout(Self& self, Stream& stream) noexcept
 *     {
 *         stream.field(self.i_tow);
 *     }
 * };
 * @endcode
 *
 * `Self` is `Derived` when decoding (through `payload_reader`) and
 * `const Derived` when encoding (through `payload_writer`); both streams expose
 * the same `field()` / `reserved()` / `constant()` verbs.
 *
 * `decode()` rejects any payload whose length is not exactly `PayloadLength`
 * and leaves `out` value-initialised on failure. `encode()` is only used by
 * messages the host sends (commands, assistance data); it is harmless on
 * pure outputs and enables loop-back tests.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_FIXED_MESSAGE_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_FIXED_MESSAGE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
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
namespace detail
{

/**
 * @brief Fixed-length UBX message base.
 *
 * @tparam Derived Message type providing `template <class Self, class Stream> static void layout(Self&, Stream&)`.
 * @tparam MsgClass UBX class byte.
 * @tparam MsgId UBX message ID byte.
 * @tparam PayloadLength Exact payload length in bytes.
 */
template <typename Derived, uint8_t MsgClass, uint8_t MsgId, castle::size_type PayloadLength>
struct fixed_message
{
    static CASTLE_CONSTEXPR uint8_t msg_class = MsgClass;
    static CASTLE_CONSTEXPR uint8_t msg_id = MsgId;
    static CASTLE_CONSTEXPR castle::size_type payload_length = PayloadLength;

    /** @brief Reports whether `raw` carries this message's class/ID. */
    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(MsgClass, MsgId);
    }

    /**
     * @brief Decodes a `PayloadLength`-byte payload into `out`.
     * @return `false` for a different class/ID, a wrong length, or a
     * constant byte that does not match; `out` is then value-initialised.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, Derived& out) CASTLE_NOEXCEPT
    {
        out = Derived{};
        if (!matches(raw) || (raw.payload_length() != PayloadLength))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        Derived::layout(out, reader);
        if (!reader.ok())
        {
            out = Derived{};
            return false;
        }
        return true;
    }

    /** @brief Writes the payload; see `encode_message()`. */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        Derived::layout(static_cast<CASTLE_CONST Derived&>(*this), writer);
    }
};

} // namespace detail
} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_FIXED_MESSAGE_HPP
