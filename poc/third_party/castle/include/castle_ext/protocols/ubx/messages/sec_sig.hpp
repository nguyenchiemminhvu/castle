// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sec_sig.hpp
 * @brief UBX-SEC-SIG (0x27 0x09): signal security (jamming / spoofing) indicator.
 *
 * Output. UBX-SEC-SIG is not part of the ZED-F9R interface description
 * (UBX-19056845); this model follows the generic u-blox version-1 layout of
 * its 12-byte prefix. Later versions append data (e.g. version 2 adds
 * jamming-frequency blocks), so `decode()` accepts any payload of at least
 * `payload_length` bytes, decodes the prefix and ignores the tail.
 * `encode()` always writes exactly `payload_length` bytes.
 *
 * The detection-enabled bits and states are exposed as accessors over the
 * raw `jam_flags` / `spf_flags` bytes rather than as separately stored
 * fields, so the decoded value cannot hold contradictory copies.
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_SIG_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_SIG_HPP

#include "castle/core/compiler.hpp"
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

/** @brief `jammingState` (bits 2..1 of `jamFlags`). */
enum class sec_sig_jamming_state : uint8_t
{
    unknown = 0U,   ///< Unknown or detection disabled.
    no_jamming = 1U,
    warning = 2U,   ///< Interference visible, fix OK.
    critical = 3U   ///< Interference visible, no fix.
};

/** @brief `spoofingState` (bits 3..1 of `spfFlags`). */
enum class sec_sig_spoofing_state : uint8_t
{
    unknown = 0U,   ///< Unknown or detection disabled.
    no_spoofing = 1U,
    indicated = 2U, ///< Spoofing indicated.
    affirmed = 3U   ///< Spoofing affirmed.
};

/** @brief UBX-SEC-SIG output payload (12-byte version-1 prefix). */
struct sec_sig : detail::fixed_message<sec_sig, UBX_CLASS_SEC, UBX_ID_SEC_SIG, 12U>
{
    /** @name `jamFlags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t JAM_FLAGS_JAM_DET_ENABLED = 0x01U; ///< `jamDetEnabled`.
    static CASTLE_CONSTEXPR uint8_t JAM_FLAGS_JAMMING_STATE = 0x06U;   ///< `jammingState`.
    /** @} */

    /** @name `spfFlags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t SPF_FLAGS_SPF_DET_ENABLED = 0x01U; ///< `spfDetEnabled`.
    static CASTLE_CONSTEXPR uint8_t SPF_FLAGS_SPOOFING_STATE = 0x0EU;  ///< `spoofingState`.
    /** @} */

    uint8_t version = 0U;   ///< Message version.
    uint8_t jam_flags = 0U; ///< Jamming information, see `JAM_FLAGS_*`.
    uint8_t spf_flags = 0U; ///< Spoofing information, see `SPF_FLAGS_*`.

    CASTLE_NODISCARD bool jam_det_enabled() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (jam_flags & JAM_FLAGS_JAM_DET_ENABLED) != 0U;
    }

    CASTLE_NODISCARD sec_sig_jamming_state jamming_state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<sec_sig_jamming_state>((jam_flags & JAM_FLAGS_JAMMING_STATE) >> 1U);
    }

    CASTLE_NODISCARD bool spf_det_enabled() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (spf_flags & SPF_FLAGS_SPF_DET_ENABLED) != 0U;
    }

    CASTLE_NODISCARD sec_sig_spoofing_state spoofing_state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<sec_sig_spoofing_state>((spf_flags & SPF_FLAGS_SPOOFING_STATE) >> 1U);
    }

    /**
     * @brief Decodes the first `payload_length` bytes of `raw` into `out`.
     * @return `false` for a different class/ID or a payload shorter than
     * `payload_length`; `out` is then value-initialised. Extra bytes are ignored.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, sec_sig& out) CASTLE_NOEXCEPT
    {
        if (raw.payload_length() < payload_length)
        {
            out = sec_sig{};
            return false;
        }
        message_view prefix = raw;
        prefix.header.payload_length = static_cast<uint16_t>(payload_length);
        prefix.payload = raw.payload.subview(0U, payload_length);
        return fixed_message::decode(prefix, out);
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.reserved(3U);
        stream.field(self.jam_flags);
        stream.reserved(3U);
        stream.field(self.spf_flags);
        stream.reserved(3U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_SEC_SIG_HPP
