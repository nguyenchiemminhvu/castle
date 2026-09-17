// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// Compile-time mathematical, character-encoding, and serialization constants.
// Include this header when deterministic Castle code needs lightweight numeric
// constants, angle-conversion helpers, ASCII and UTF boundary values, or shared
// serialization limits without pulling in the STL. All values are constexpr,
// require no runtime initialization, and are designed for heap-free embedded
// builds.

#ifndef CASTLE_CORE_CONSTANTS_HPP
#define CASTLE_CORE_CONSTANTS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/traits.hpp"

namespace castle
{

/// @brief Mathematical constants and angle-conversion helpers.
namespace math
{

/// @brief Returns pi in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return Pi converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
pi() CASTLE_NOEXCEPT
{
    return static_cast<T>(3.141592653589793238462643383279502884L);
}

/// @brief Returns tau in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return Two pi converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
tau() CASTLE_NOEXCEPT
{
    return static_cast<T>(6.283185307179586476925286766559005768L);
}

/// @brief Converts degrees to radians.
/// @tparam T Floating-point operand type.
/// @param degrees Angle value in degrees.
/// @return The converted angle in radians.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
degrees_to_radians(T degrees) CASTLE_NOEXCEPT
{
    return degrees * pi<T>() / static_cast<T>(180);
}

/// @brief Converts radians to degrees.
/// @tparam T Floating-point operand type.
/// @param radians Angle value in radians.
/// @return The converted angle in degrees.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
radians_to_degrees(T radians) CASTLE_NOEXCEPT
{
    return radians * static_cast<T>(180) / pi<T>();
}

/// @brief Returns Euler's number in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return Euler's number converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
e() CASTLE_NOEXCEPT
{
    return static_cast<T>(2.71828182845904523536L);
}

/// @brief Returns the golden ratio in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return The golden ratio converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
golden_ratio() CASTLE_NOEXCEPT
{
    return static_cast<T>(1.61803398874989484820L);
}

/// @brief Returns log2(e) in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return log2(e) converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
log2_e() CASTLE_NOEXCEPT
{
    return static_cast<T>(1.44269504088896340736L);
}

/// @brief Returns log10(e) in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return log10(e) converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
log10_e() CASTLE_NOEXCEPT
{
    return static_cast<T>(0.43429448190325182765L);
}

/// @brief Returns ln(2) in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return Natural logarithm of 2 converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
ln_2() CASTLE_NOEXCEPT
{
    return static_cast<T>(0.69314718055994530942L);
}

/// @brief Returns ln(10) in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return Natural logarithm of 10 converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
ln_10() CASTLE_NOEXCEPT
{
    return static_cast<T>(2.30258509299404568402L);
}

/// @brief Returns the square root of 2 in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return Square root of 2 converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
sqrt_2() CASTLE_NOEXCEPT
{
    return static_cast<T>(1.41421356237309504880L);
}

/// @brief Returns the square root of 3 in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return Square root of 3 converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
sqrt_3() CASTLE_NOEXCEPT
{
    return static_cast<T>(1.73205080756887729352L);
}

/// @brief Returns the reciprocal of the square root of 2 in the requested floating-point type.
/// @tparam T Floating-point result type.
/// @return Inverse square root of 2 converted to T.
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
inv_sqrt_2() CASTLE_NOEXCEPT
{
    return static_cast<T>(0.70710678118654752440L);
}

} // namespace math

/// @brief Character-set boundary values shared by text and serialization code.
namespace characters
{

inline CASTLE_CONSTEXPR uint8_t  ascii_control_min = 0x00U; ///< Minimum ASCII control character
inline CASTLE_CONSTEXPR uint8_t  ascii_control_max = 0x20U; ///< Maximum ASCII control character
inline CASTLE_CONSTEXPR uint8_t  ascii_lower_min = 0x61U; ///< Minimum ASCII lowercase letter 'a'
inline CASTLE_CONSTEXPR uint8_t  ascii_lower_max = 0x7AU; ///< Maximum ASCII lowercase letter 'z'
inline CASTLE_CONSTEXPR uint8_t  ascii_upper_min = 0x41U; ///< Minimum ASCII uppercase letter 'A'
inline CASTLE_CONSTEXPR uint8_t  ascii_upper_max = 0x5AU; ///< Maximum ASCII uppercase letter 'Z'
inline CASTLE_CONSTEXPR uint8_t  ascii_digit_min = 0x30U; ///< Minimum ASCII digit '0'
inline CASTLE_CONSTEXPR uint8_t  ascii_digit_max = 0x39U; ///< Maximum ASCII digit '9'

inline CASTLE_CONSTEXPR uint8_t  utf8_shift_6 = 6U; ///< Number of bits to shift for 6-bit UTF-8 sequences
inline CASTLE_CONSTEXPR uint8_t  utf8_shift_12 = 12U; ///< Number of bits to shift for 12-bit UTF-8 sequences
inline CASTLE_CONSTEXPR uint8_t  utf8_shift_18 = 18U; ///< Number of bits to shift for 18-bit UTF-8 sequences
inline CASTLE_CONSTEXPR uint8_t  utf8_ascii_max = 0x7FU; ///< Maximum ASCII value for UTF-8
inline CASTLE_CONSTEXPR uint8_t  utf8_lead2_min = 0xC2U; ///< Minimum lead byte for 2-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_lead2_max = 0xDFU; ///< Maximum lead byte for 2-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_lead3_min = 0xE0U; ///< Minimum lead byte for 3-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_lead3_max = 0xEFU; ///< Maximum lead byte for 3-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_lead4_min = 0xF0U; ///< Minimum lead byte for 4-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_lead4_max = 0xF4U; ///< Maximum lead byte for 4-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_two_byte_prefix = 0xC0U; ///< Prefix for 2-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_three_byte_prefix = 0xE0U; ///< Prefix for 3-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_four_byte_prefix = 0xF0U; ///< Prefix for 4-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_continuation_prefix = 0x80U; ///< Prefix for UTF-8 continuation byte
inline CASTLE_CONSTEXPR uint8_t  utf8_continuation_mask = 0x3FU; ///< Mask for UTF-8 continuation byte
inline CASTLE_CONSTEXPR uint8_t  utf8_decode_mask_2 = 0x1FU; ///< Decode mask for 2-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_decode_mask_3 = 0x0FU; ///< Decode mask for 3-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint8_t  utf8_decode_mask_4 = 0x07U; ///< Decode mask for 4-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint16_t utf8_one_byte_max = 0x007FU; ///< Maximum code point for 1-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint16_t utf8_two_byte_max = 0x07FFU; ///< Maximum code point for 2-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint32_t utf8_three_byte_max = 0xFFFFU; ///< Maximum code point for 3-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint32_t utf8_min_3_byte_codepoint = 0x800U; ///< Minimum code point for 3-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint32_t utf8_min_4_byte_codepoint = 0x10000U; ///< Minimum code point for 4-byte UTF-8 sequence
inline CASTLE_CONSTEXPR uint16_t utf16_surrogate_min = 0xD800U; ///< Minimum UTF-16 surrogate code unit
inline CASTLE_CONSTEXPR uint16_t utf16_surrogate_max = 0xDFFFU; ///< Maximum UTF-16 surrogate code unit

} // namespace characters

/// @brief Shared size limits and code-point ranges for Castle serializers.
namespace serialization
{

inline CASTLE_CONSTEXPR uint8_t utf8_bom_byte_1 = 0xEFU; ///< First byte of UTF-8 BOM (Byte Order Mark)
inline CASTLE_CONSTEXPR uint8_t utf8_bom_byte_2 = 0xBBU; ///< Second byte of UTF-8 BOM (Byte Order Mark)
inline CASTLE_CONSTEXPR uint8_t utf8_bom_byte_3 = 0xBFU; ///< Third byte of UTF-8 BOM (Byte Order Mark)

inline CASTLE_CONSTEXPR size_type max_integer_str_len = 32U; ///< Maximum length of integer string representation
inline CASTLE_CONSTEXPR size_type max_floating_str_len = 48U; ///< Maximum length of floating-point string representation
inline CASTLE_CONSTEXPR uint32_t  unicode_max_codepoint = 0x10FFFFU; ///< Maximum valid Unicode code point
inline CASTLE_CONSTEXPR int        max_floating_exponent = 1024; ///< Maximum exponent for floating-point numbers

inline CASTLE_CONSTEXPR uint32_t xml_tab = 0x09U; ///< XML tab character
inline CASTLE_CONSTEXPR uint32_t xml_line_feed = 0x0AU; ///< XML line feed character
inline CASTLE_CONSTEXPR uint32_t xml_carriage_return = 0x0DU; ///< XML carriage return character
inline CASTLE_CONSTEXPR uint32_t xml_char_min = 0x20U; ///< Minimum valid XML character
inline CASTLE_CONSTEXPR uint32_t xml_bmp_first_max = 0xD7FFU; ///< Maximum code point of the first BMP range in XML
inline CASTLE_CONSTEXPR uint32_t xml_bmp_second_min = 0xE000U; ///< Minimum code point of the second BMP range in XML
inline CASTLE_CONSTEXPR uint32_t xml_bmp_second_max = 0xFFFDU; ///< Maximum code point of the second BMP range in XML
inline CASTLE_CONSTEXPR uint32_t xml_supplementary_min = 0x10000U; ///< Minimum supplementary code point in XML
inline CASTLE_CONSTEXPR uint32_t xml_supplementary_max = 0x10FFFFU; ///< Maximum supplementary code point in XML

} // namespace serialization

} // namespace castle

#endif // CASTLE_CORE_CONSTANTS_HPP
