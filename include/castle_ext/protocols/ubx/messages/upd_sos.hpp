// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file upd_sos.hpp
 * @brief UBX-UPD-SOS (0x09 0x14): save-on-shutdown backup in flash.
 *
 * One class/ID carries five layouts, told apart by payload byte 0 (`cmd`)
 * and the exact length:
 *  - poll (empty payload): use `encode_poll<upd_sos_restored>()`; the
 *    receiver answers with `upd_sos_restored`;
 *  - `upd_sos_save` (cmd 0, 4 bytes) and `upd_sos_clear` (cmd 1, 4 bytes):
 *    commands sent by the host;
 *  - `upd_sos_ack` (cmd 2, 8 bytes): backup creation acknowledge;
 *  - `upd_sos_restored` (cmd 3, 8 bytes): system restored from backup.
 *
 * The `response` byte means different things in the two outputs, hence the
 * two enumerations `upd_sos_ack_response` and `upd_sos_restore_response`.
 *
 * @code
 * castle::protocols::ubx::messages::upd_sos_save save{};
 * uint8_t frame[12U];
 * castle::size_type written = 0U;
 * castle::status result = castle::protocols::ubx::encode_message(save, frame, sizeof(frame), written);
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_UPD_SOS_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_UPD_SOS_HPP

#include "castle/core/compiler.hpp"
#include "castle_ext/protocols/ubx/ubx.hpp"
#include "castle_ext/protocols/ubx/messages/detail/typed_message.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace ubx
{
namespace messages
{

/** @brief `cmd` byte: selects the UBX-UPD-SOS layout. */
enum class upd_sos_cmd : uint8_t
{
    save = 0x00U,     ///< Create backup in flash (command).
    clear = 0x01U,    ///< Clear backup in flash (command).
    ack = 0x02U,      ///< Backup creation acknowledge (output).
    restored = 0x03U  ///< System restored from backup (output).
};

/** @brief `response` of the backup creation acknowledge (`upd_sos_ack`). */
enum class upd_sos_ack_response : uint8_t
{
    not_acknowledged = 0x00U,
    acknowledged = 0x01U
};

/** @brief `response` of the system-restored output (`upd_sos_restored`). */
enum class upd_sos_restore_response : uint8_t
{
    unknown = 0x00U,
    failed = 0x01U,       ///< Failed restoring from backup.
    restored = 0x02U,     ///< Restored from backup.
    not_restored = 0x03U  ///< Not restored (no backup).
};

/** @brief UBX-UPD-SOS "create backup in flash" command (4 bytes). */
struct upd_sos_save
    : detail::typed_message<upd_sos_save, UBX_CLASS_UPD, UBX_ID_UPD_SOS, static_cast<uint8_t>(upd_sos_cmd::save), 4U>
{
    template <typename Self, typename Stream>
    static void layout(Self&, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(static_cast<uint8_t>(upd_sos_cmd::save));
        stream.reserved(3U);
    }
};

/** @brief UBX-UPD-SOS "clear backup in flash" command (4 bytes). */
struct upd_sos_clear
    : detail::typed_message<upd_sos_clear, UBX_CLASS_UPD, UBX_ID_UPD_SOS, static_cast<uint8_t>(upd_sos_cmd::clear), 4U>
{
    template <typename Self, typename Stream>
    static void layout(Self&, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(static_cast<uint8_t>(upd_sos_cmd::clear));
        stream.reserved(3U);
    }
};

/** @brief UBX-UPD-SOS backup creation acknowledge output (8 bytes). */
struct upd_sos_ack
    : detail::typed_message<upd_sos_ack, UBX_CLASS_UPD, UBX_ID_UPD_SOS, static_cast<uint8_t>(upd_sos_cmd::ack), 8U>
{
    upd_sos_ack_response response = upd_sos_ack_response::not_acknowledged;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(static_cast<uint8_t>(upd_sos_cmd::ack));
        stream.reserved(3U);
        stream.field(self.response);
        stream.reserved(3U);
    }
};

/** @brief UBX-UPD-SOS system-restored-from-backup output (8 bytes); also the poll answer. */
struct upd_sos_restored
    : detail::typed_message<upd_sos_restored, UBX_CLASS_UPD, UBX_ID_UPD_SOS, static_cast<uint8_t>(upd_sos_cmd::restored), 8U>
{
    upd_sos_restore_response response = upd_sos_restore_response::unknown;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(static_cast<uint8_t>(upd_sos_cmd::restored));
        stream.reserved(3U);
        stream.field(self.response);
        stream.reserved(3U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_UPD_SOS_HPP
