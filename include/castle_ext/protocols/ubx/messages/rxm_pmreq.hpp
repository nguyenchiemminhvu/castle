// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file rxm_pmreq.hpp
 * @brief UBX-RXM-PMREQ (0x02 0x41): power management request.
 *
 * Command sent by the host. Two layouts share the class/ID and differ only
 * by payload length (there is no type byte): the 8-byte `rxm_pmreq_v0`
 * (duration + backup flag) and the 16-byte `rxm_pmreq_v1` (adds the version
 * byte, the `force` flag and wake-up sources). Each narrows `matches()` to
 * its exact length.
 *
 * @code
 * castle::protocols::ubx::messages::rxm_pmreq_v1 request{};
 * request.duration = 60000U;
 * request.flags = rxm_pmreq_v1::FLAGS_BACKUP;
 * request.wakeup_sources = rxm_pmreq_v1::WAKEUP_UARTRX | rxm_pmreq_v1::WAKEUP_EXTINT0;
 * uint8_t frame[24U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(request, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_PMREQ_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_PMREQ_HPP

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

/** @brief `duration` value requesting an infinite task duration. */
static CASTLE_CONSTEXPR uint32_t RXM_PMREQ_DURATION_INFINITE = 0U;

/** @brief Message version byte of the 16-byte layout. */
static CASTLE_CONSTEXPR uint8_t RXM_PMREQ_VERSION = 0x00U;

/** @brief UBX-RXM-PMREQ 8-byte command payload (no version, no wake-up sources). */
struct rxm_pmreq_v0 : detail::fixed_message<rxm_pmreq_v0, UBX_CLASS_RXM, UBX_ID_RXM_PMREQ, 8U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint32_t FLAGS_BACKUP = 0x00000002U; ///< `backup`: enter backup mode for `duration`.
    /** @} */

    uint32_t duration = RXM_PMREQ_DURATION_INFINITE; ///< Task duration, 0 = infinite, at most 12 days [ms].
    uint32_t flags = 0U;                             ///< Task flags, see `FLAGS_*`.

    /** @brief Reports whether `raw` has this class/ID and exactly 8 payload bytes. */
    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id) && (raw.payload_length() == payload_length);
    }

    CASTLE_NODISCARD bool backup() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_BACKUP) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.duration);
        stream.field(self.flags);
    }
};

/** @brief UBX-RXM-PMREQ 16-byte command payload (version 0x00, with wake-up sources). */
struct rxm_pmreq_v1 : detail::fixed_message<rxm_pmreq_v1, UBX_CLASS_RXM, UBX_ID_RXM_PMREQ, 16U>
{
    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint32_t FLAGS_BACKUP = 0x00000002U; ///< `backup`: enter backup mode for `duration`.
    static CASTLE_CONSTEXPR uint32_t FLAGS_FORCE = 0x00000004U;  ///< `force`: back up even with USB connected.
    /** @} */

    /** @name `wakeupSources` bits @{ */
    static CASTLE_CONSTEXPR uint32_t WAKEUP_UARTRX = 0x00000008U;  ///< Edge on UART RX.
    static CASTLE_CONSTEXPR uint32_t WAKEUP_EXTINT0 = 0x00000020U; ///< Edge on EXTINT0.
    static CASTLE_CONSTEXPR uint32_t WAKEUP_EXTINT1 = 0x00000040U; ///< Edge on EXTINT1.
    static CASTLE_CONSTEXPR uint32_t WAKEUP_SPICS = 0x00000080U;   ///< Edge on SPI CS.
    /** @} */

    uint32_t duration = RXM_PMREQ_DURATION_INFINITE; ///< Task duration, 0 = infinite, at most 12 days [ms].
    uint32_t flags = 0U;                             ///< Task flags, see `FLAGS_*`.
    uint32_t wakeup_sources = 0U;                    ///< Wake-up pins, see `WAKEUP_*`.

    /** @brief Reports whether `raw` has this class/ID and exactly 16 payload bytes. */
    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id) && (raw.payload_length() == payload_length);
    }

    CASTLE_NODISCARD bool backup() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_BACKUP) != 0U;
    }

    CASTLE_NODISCARD bool force() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_FORCE) != 0U;
    }

    CASTLE_NODISCARD bool wakeup_uartrx() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (wakeup_sources & WAKEUP_UARTRX) != 0U;
    }

    CASTLE_NODISCARD bool wakeup_extint0() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (wakeup_sources & WAKEUP_EXTINT0) != 0U;
    }

    CASTLE_NODISCARD bool wakeup_extint1() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (wakeup_sources & WAKEUP_EXTINT1) != 0U;
    }

    CASTLE_NODISCARD bool wakeup_spics() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (wakeup_sources & WAKEUP_SPICS) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(RXM_PMREQ_VERSION);
        stream.reserved(3U);
        stream.field(self.duration);
        stream.field(self.flags);
        stream.field(self.wakeup_sources);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_RXM_PMREQ_HPP
