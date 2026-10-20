// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mon_hw3.hpp
 * @brief UBX-MON-HW3 (0x0a 0x37): I/O pin status.
 *
 * Periodic/polled output: a 22-byte header followed by `nPins` 6-byte pin
 * blocks (`22 + nPins·6`). Antenna and RF status live in UBX-MON-RF
 * (`mon_rf.hpp`).
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_HW3_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_HW3_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string.hpp"
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

/** @brief One I/O pin block of UBX-MON-HW3. */
struct mon_hw3_pin
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 6U;

    /** @name `pinMask` bits @{ */
    static CASTLE_CONSTEXPR uint16_t PIN_MASK_PERIPH_PIO = 0x0001U;    ///< 0 = peripheral, 1 = PIO.
    static CASTLE_CONSTEXPR uint16_t PIN_MASK_PIN_BANK = 0x000EU;      ///< Bank 0..7 = A..H.
    static CASTLE_CONSTEXPR uint16_t PIN_MASK_DIRECTION = 0x0010U;     ///< 0 = input, 1 = output.
    static CASTLE_CONSTEXPR uint16_t PIN_MASK_VALUE = 0x0020U;         ///< 0 = low, 1 = high.
    static CASTLE_CONSTEXPR uint16_t PIN_MASK_VP_MANAGER = 0x0040U;    ///< Used by the virtual pin manager.
    static CASTLE_CONSTEXPR uint16_t PIN_MASK_PIO_IRQ = 0x0080U;       ///< Interrupt enabled.
    static CASTLE_CONSTEXPR uint16_t PIN_MASK_PIO_PULL_HIGH = 0x0100U; ///< Pull-high resistor in use.
    static CASTLE_CONSTEXPR uint16_t PIN_MASK_PIO_PULL_LOW = 0x0200U;  ///< Pull-low resistor in use.
    /** @} */

    uint16_t pin_id = 0U;   ///< Pin identifier (external and internal pins).
    uint16_t pin_mask = 0U;
    uint8_t vp = 0U;        ///< Virtual pin mapping.

    CASTLE_NODISCARD bool periph_pio() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (pin_mask & PIN_MASK_PERIPH_PIO) != 0U;
    }

    /** @brief Pin bank (0 = A ... 7 = H). */
    CASTLE_NODISCARD uint8_t pin_bank() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<uint8_t>((pin_mask & PIN_MASK_PIN_BANK) >> 1U);
    }

    CASTLE_NODISCARD bool direction() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (pin_mask & PIN_MASK_DIRECTION) != 0U;
    }

    CASTLE_NODISCARD bool value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (pin_mask & PIN_MASK_VALUE) != 0U;
    }

    CASTLE_NODISCARD bool vp_manager() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (pin_mask & PIN_MASK_VP_MANAGER) != 0U;
    }

    CASTLE_NODISCARD bool pio_irq() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (pin_mask & PIN_MASK_PIO_IRQ) != 0U;
    }

    CASTLE_NODISCARD bool pio_pull_high() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (pin_mask & PIN_MASK_PIO_PULL_HIGH) != 0U;
    }

    CASTLE_NODISCARD bool pio_pull_low() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (pin_mask & PIN_MASK_PIO_PULL_LOW) != 0U;
    }

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.pin_id);
        stream.field(self.pin_mask);
        stream.field(self.vp);
        stream.reserved(1U);
    }
};

/** @brief UBX-MON-HW3 output payload. */
template <castle::size_type MaxPins = 32U>
struct mon_hw3
{
    static_assert(MaxPins <= 255U, "nPins is a U1 field");

    static CASTLE_CONSTEXPR uint8_t msg_class = UBX_CLASS_MON;
    static CASTLE_CONSTEXPR uint8_t msg_id = UBX_ID_MON_HW3;
    static CASTLE_CONSTEXPR castle::size_type header_length = 22U;
    static CASTLE_CONSTEXPR castle::size_type block_length = mon_hw3_pin::wire_size;

    /** @brief Width of the `hwVersion` field (`CH[10]`). */
    static CASTLE_CONSTEXPR castle::size_type HW_VERSION_LENGTH = 10U;

    /** @name `flags` bits @{ */
    static CASTLE_CONSTEXPR uint8_t FLAGS_RTC_CALIB = 0x01U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_SAFE_BOOT = 0x02U;
    static CASTLE_CONSTEXPR uint8_t FLAGS_XTAL_ABSENT = 0x04U;
    /** @} */

    uint8_t version = 0U;  ///< Message version (0x00).
    uint8_t n_pins = 0U;   ///< On-wire pin count; may exceed `pins.size()`.
    uint8_t flags = 0U;
    castle::container::string<HW_VERSION_LENGTH> hw_version{}; ///< Hardware version (as in UBX-MON-VER).
    castle::container::vector<mon_hw3_pin, MaxPins> pins{};

    CASTLE_NODISCARD bool rtc_calib() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_RTC_CALIB) != 0U;
    }

    CASTLE_NODISCARD bool safe_boot() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_SAFE_BOOT) != 0U;
    }

    CASTLE_NODISCARD bool xtal_absent() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (flags & FLAGS_XTAL_ABSENT) != 0U;
    }

    CASTLE_NODISCARD static bool matches(CASTLE_CONST message_view& raw) CASTLE_NOEXCEPT
    {
        return raw.is(msg_class, msg_id);
    }

    /**
     * @brief Decodes the header and up to `MaxPins` pin blocks.
     * @return `false` unless the payload length is exactly `22 + nPins·6`.
     */
    CASTLE_NODISCARD static bool decode(CASTLE_CONST message_view& raw, mon_hw3& out) CASTLE_NOEXCEPT
    {
        out = mon_hw3{};
        if (!matches(raw) || (raw.payload_length() < header_length))
        {
            return false;
        }

        payload_reader reader(raw.payload);
        layout_header(out, out.n_pins, reader);
        if (!detail::has_group_length(raw.payload_length(), header_length, out.n_pins, block_length))
        {
            out = mon_hw3{};
            return false;
        }

        detail::read_group(reader, out.n_pins, out.pins);
        return reader.ok();
    }

    /** @brief Writes the header and every stored pin (`nPins` is taken from `pins.size()`). */
    void encode(payload_writer& writer) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint8_t count = static_cast<uint8_t>(pins.size());
        layout_header(*this, count, writer);
        detail::write_group(writer, pins);
    }

private:
    template <typename Self, typename Count, typename Stream>
    static void layout_header(Self& self, Count& count, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.version);
        stream.field(count);
        stream.field(self.flags);
        stream.text(self.hw_version);
        stream.reserved(9U);
    }
};

} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_MON_HW3_HPP
