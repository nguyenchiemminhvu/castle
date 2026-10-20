// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file esf_data.hpp
 * @brief Shared codec for the UBX-ESF sensor data word (`X4 data`).
 *
 * UBX-ESF-MEAS and UBX-ESF-RAW carry measurements in the same 32-bit word:
 * bits 23..0 hold `dataField`, a two's-complement 24-bit value, and the bits
 * above hold `dataType` (bits 29..24 in ESF-MEAS, bits 31..24 in ESF-RAW).
 *
 * `esf_data_word<TypeMask>()` is a layout verb overloaded on the stream, so a
 * block describes the word once for both directions:
 *
 * @code
 * template <typename Self, typename Stream>
 * static void layout(Self& self, Stream& stream) noexcept
 * {
 *     detail::esf_data_word<0x3FU>(stream, self.data_type, self.data_value);
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_ESF_DATA_HPP
#define CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_ESF_DATA_HPP

#include "castle/core/compiler.hpp"
#include "castle/error/status.hpp"
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

/** @brief `dataField` bits 23..0 of an ESF data word. */
static CASTLE_CONSTEXPR uint32_t ESF_DATA_FIELD_MASK = 0x00FFFFFFU;
/** @brief Sign bit (bit 23) of `dataField`. */
static CASTLE_CONSTEXPR uint32_t ESF_DATA_FIELD_SIGN = 0x00800000U;
/** @brief Bit position of `dataType`. */
static CASTLE_CONSTEXPR uint32_t ESF_DATA_TYPE_SHIFT = 24U;
/** @brief Smallest value representable in the signed 24-bit `dataField`. */
static CASTLE_CONSTEXPR int32_t ESF_DATA_FIELD_MIN = -0x00800000;
/** @brief Largest value representable in the signed 24-bit `dataField`. */
static CASTLE_CONSTEXPR int32_t ESF_DATA_FIELD_MAX = 0x007FFFFF;

/** @brief Returns `dataField` (bits 23..0 of `word`) sign-extended to 32 bits. */
CASTLE_NODISCARD CASTLE_CONSTEXPR int32_t esf_data_field(uint32_t word) CASTLE_NOEXCEPT
{
    return static_cast<int32_t>(word & ESF_DATA_FIELD_MASK)
        - (((word & ESF_DATA_FIELD_SIGN) != 0U) ? static_cast<int32_t>(ESF_DATA_FIELD_MASK + 1U) : 0);
}

/**
 * @brief Decodes one ESF data word into its type and sign-extended value.
 * @tparam TypeMask `dataType` mask after shifting (0x3F for ESF-MEAS, 0xFF for ESF-RAW).
 */
template <uint8_t TypeMask>
CASTLE_INLINE void esf_data_word(payload_reader& reader, uint8_t& data_type, int32_t& data_field) CASTLE_NOEXCEPT
{
    CASTLE_CONST uint32_t word = reader.read_u32();
    data_type = static_cast<uint8_t>((word >> ESF_DATA_TYPE_SHIFT) & TypeMask);
    data_field = esf_data_field(word);
}

/**
 * @brief Encodes one ESF data word.
 *
 * Latches `castle::status::out_of_range` (nothing is written) when
 * `data_type` has bits outside `TypeMask` or `data_field` does not fit in a
 * signed 24-bit value, so a bad input never reaches the wire truncated.
 */
template <uint8_t TypeMask>
CASTLE_INLINE void esf_data_word(
    payload_writer& writer,
    CASTLE_CONST uint8_t& data_type,
    CASTLE_CONST int32_t& data_field) CASTLE_NOEXCEPT
{
    if (((data_type & static_cast<uint8_t>(~TypeMask)) != 0U)
        || (data_field < ESF_DATA_FIELD_MIN)
        || (data_field > ESF_DATA_FIELD_MAX))
    {
        writer.fail(castle::status::out_of_range);
        return;
    }
    writer.write(static_cast<uint32_t>((static_cast<uint32_t>(data_type) << ESF_DATA_TYPE_SHIFT)
                                       | (static_cast<uint32_t>(data_field) & ESF_DATA_FIELD_MASK)));
}

} // namespace detail
} // namespace messages
} // namespace ubx
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_UBX_MESSAGES_DETAIL_ESF_DATA_HPP
