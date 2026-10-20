// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sec_uniqid.hpp
 * @brief UBX-SEC-UNIQID (0x27 0x03): unique chip ID.
 *
 * Polled output carrying the 40-bit (5-byte) unique chip identifier. This
 * model follows UBX-19056845 exactly (9-byte payload); frames with a
 * different length are rejected.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_UNIQID_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_UNIQID_HPP

#include "castle/core/compiler.hpp"
#include "castle/container/array.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/detail/fixed_message.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief UBX-SEC-UNIQID output payload. */
struct sec_uniqid : detail::fixed_message<sec_uniqid, UBX_CLASS_SEC, UBX_ID_SEC_UNIQID, 9U>
{
    uint8_t version = 0U;                              ///< Message version (0x01 for this version).
    castle::container::array<uint8_t, 5U> unique_id{}; ///< Unique chip ID, in wire order.

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.reserved(3U);
        stream.field(self.unique_id);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_UNIQID_HPP
