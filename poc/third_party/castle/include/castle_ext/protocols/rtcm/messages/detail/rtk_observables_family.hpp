// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file rtk_observables_family.hpp
 * @brief Compile-time layout of one legacy RTK observables message (1001-1004, 1009-1012).
 *
 * Both legacy observables families are four consecutive message numbers with
 * the same structure:
 *
 * | Offset from `First` | Content             | `IS_EXTENDED` (ambiguity, CNR) | `HAS_L2` |
 * |---------------------|---------------------|--------------------------------|----------|
 * | 0                   | L1-only             | false                          | false    |
 * | 1                   | L1-only, extended   | true                           | false    |
 * | 2                   | L1 and L2           | false                          | true     |
 * | 3                   | L1 and L2, extended | true                           | true     |
 *
 * `gps_rtk_observables<Type>` (First = 1001) and `glonass_rtk_observables<Type>`
 * (First = 1009) inherit these constants instead of spelling them twice.
 */
#ifndef CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_DETAIL_RTK_OBSERVABLES_FAMILY_HPP
#define CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_DETAIL_RTK_OBSERVABLES_FAMILY_HPP

#include "castle/core/compiler.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace rtcm
{
namespace messages
{
namespace detail
{

/**
 * @brief Message number and optional-field selection of one legacy observables message.
 *
 * @tparam First L1-only message number of the family (1001 for GPS, 1009 for GLONASS).
 * @tparam Type  Message number of the model, `First .. First + 3`.
 */
template <uint16_t First, uint16_t Type>
struct rtk_observables_family
{
    /** @brief L1-only message number. */
    static CASTLE_CONSTEXPR uint16_t FIRST_TYPE = First;
    /** @brief Extended L1/L2 message number, the last of the family. */
    static CASTLE_CONSTEXPR uint16_t LAST_TYPE = static_cast<uint16_t>(First + 3U);

    static_assert((Type >= FIRST_TYPE) && (Type <= LAST_TYPE), "message type must be one of First .. First + 3");

    /** @brief Message number (DF002) of the model. */
    static CASTLE_CONSTEXPR uint16_t MESSAGE_TYPE = Type;
    /** @brief The message carries the ambiguity and CNR fields (second and fourth number). */
    static CASTLE_CONSTEXPR bool IS_EXTENDED = (((Type - First) & 1U) != 0U);
    /** @brief The message carries L2 fields (third and fourth number). */
    static CASTLE_CONSTEXPR bool HAS_L2 = ((Type - First) >= 2U);

    /** @brief Reports whether `type` is this message's number. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR bool accepts(uint16_t type) CASTLE_NOEXCEPT
    {
        return type == MESSAGE_TYPE;
    }
};

} // namespace detail
} // namespace messages
} // namespace rtcm
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_RTCM_MESSAGES_DETAIL_RTK_OBSERVABLES_FAMILY_HPP
