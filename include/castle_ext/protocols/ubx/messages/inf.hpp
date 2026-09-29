// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_INF_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_INF_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle/container/string_view.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"

namespace castle_ext
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-INF subtype (severity) carried in the message ID. */
enum class inf_subtype : uint8_t
{
    error = 0x00U,
    warning = 0x01U,
    notice = 0x02U,
    test = 0x03U,
    debug = 0x04U
};

/** @brief UBX-INF-*: textual information/diagnostic message. */
template <castle::size_type MaxText = 256U>
struct inf
{
    uint8_t msg_class = UBX_CLASS_INF;
    uint8_t message_id = 0U;
    inf_subtype subtype = inf_subtype::error;
    castle::container::array<char, MaxText> text_storage{};
    castle::size_type text_length = 0U;

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return (raw.msg_class() == UBX_CLASS_INF)
            && ((raw.msg_id() >= UBX_ID_INF_ERROR) && (raw.msg_id() <= UBX_ID_INF_DEBUG));
    }

    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, inf& out) CASTLE_NOEXCEPT
    {
        if (!matches(raw))
        {
            return false;
        }

        out = inf{};
        out.message_id = raw.msg_id();
        out.subtype = static_cast<inf_subtype>(raw.msg_id());
        castle::size_type count = (raw.payload.size() < MaxText) ? raw.payload.size() : MaxText;
        out.text_length = 0U;
        for (castle::size_type index = 0U; index < count; ++index)
        {
            if (raw.payload[index] == 0U)
            {
                break;
            }
            out.text_storage[out.text_length++] = static_cast<char>(raw.payload[index]);
        }

        return true;
    }

    CASTLE_NODISCARD castle::container::string_view text() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::container::string_view(text_storage.data(), text_length);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle_ext

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_INF_HPP
