// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file nav_geofence.hpp
 * @brief UBX-NAV-GEOFENCE (0x01 0x39): geofencing status.
 *
 * Periodic/polled output: an 8-byte header followed by `numFences` 2-byte
 * fence blocks (`8 + numFences·2`).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_GEOFENCE_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_GEOFENCE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/vector.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/payload_reader.hpp"
#include "castle_ext/protocols/ubx/payload_writer.hpp"
#include "castle_ext/protocols/ubx/messages/detail/repeated_group.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief Geofence state relative to one fence, or the combined state. */
enum class nav_geofence_state : uint8_t
{
    unknown = 0U,
    inside = 1U,
    outside = 2U
};

/** @brief One fence block of UBX-NAV-GEOFENCE. */
struct nav_geofence_fence
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 2U;

    nav_geofence_state state = nav_geofence_state::unknown;
    uint8_t id = 0U; ///< Fence ID (0 = not available).

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.state);
        stream.field(self.id);
    }
};

/** @brief UBX-NAV-GEOFENCE output payload. */
template <castle::size_type MaxFences = 4U>
struct nav_geofence
{
    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_NAV;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_NAV_GEOFENCE;
    static CASTLE_CONSTEXPR castle::size_type header_length = 8U;
    static CASTLE_CONSTEXPR castle::size_type block_length = nav_geofence_fence::wire_size;

    /** @brief `status` values. */
    static CASTLE_CONSTEXPR uint8_t STATUS_NOT_AVAILABLE = 0U;
    static CASTLE_CONSTEXPR uint8_t STATUS_ACTIVE = 1U;

    uint32_t i_tow = 0U;
    uint8_t version = 0U;
    uint8_t status = STATUS_NOT_AVAILABLE;
    uint8_t num_fences = 0U; ///< On-wire fence count; may exceed `fences.size()`.
    nav_geofence_state comb_state = nav_geofence_state::unknown;
    castle::container::vector<nav_geofence_fence, MaxFences> fences{};

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxFences` fence blocks.
     * @return `false` unless the payload length is exactly `8 + numFences·2`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, nav_geofence& out) CASTLE_NOEXCEPT
    {
        out = nav_geofence{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.num_fences, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.num_fences, block_length))
        {
            out = nav_geofence{};
            return false;
        }

        detail::read_group(reader, out.num_fences, out.fences);
        return reader.ok();
    }

    /** @brief Writes the header and every stored fence (`numFences` is taken from `fences.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t count = static_cast<uint8_t>(fences.size());
        layout_header(*this, count, writer);
        detail::write_group(writer, fences);
    }

private:
    /**
     * @brief Header layout; `count` is the wire `numFences` (the decoded field
     * on read, `fences.size()` on write) so encoding never copies the message.
     */
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& count, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
        stream.field(self.version);
        stream.field(self.status);
        stream.field(count);
        stream.field(self.comb_state);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_NAV_GEOFENCE_HPP
