#ifndef CASTLE_CORE_CONSTANTS_HPP
#define CASTLE_CORE_CONSTANTS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/traits.hpp"

namespace castle
{

namespace math
{

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
pi() CASTLE_NOEXCEPT
{
    return static_cast<T>(3.141592653589793238462643383279502884L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
tau() CASTLE_NOEXCEPT
{
    return static_cast<T>(6.283185307179586476925286766559005768L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
degrees_to_radians(T degrees) CASTLE_NOEXCEPT
{
    return degrees * pi<T>() / static_cast<T>(180);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
radians_to_degrees(T radians) CASTLE_NOEXCEPT
{
    return radians * static_cast<T>(180) / pi<T>();
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
e() CASTLE_NOEXCEPT
{
    return static_cast<T>(2.71828182845904523536L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
golden_ratio() CASTLE_NOEXCEPT
{
    return static_cast<T>(1.61803398874989484820L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
log2_e() CASTLE_NOEXCEPT
{
    return static_cast<T>(1.44269504088896340736L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
log10_e() CASTLE_NOEXCEPT
{
    return static_cast<T>(0.43429448190325182765L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
ln_2() CASTLE_NOEXCEPT
{
    return static_cast<T>(0.69314718055994530942L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
ln_10() CASTLE_NOEXCEPT
{
    return static_cast<T>(2.30258509299404568402L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
sqrt_2() CASTLE_NOEXCEPT
{
    return static_cast<T>(1.41421356237309504880L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
sqrt_3() CASTLE_NOEXCEPT
{
    return static_cast<T>(1.73205080756887729352L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
inv_sqrt_2() CASTLE_NOEXCEPT
{
    return static_cast<T>(0.70710678118654752440L);
}

} // namespace math

namespace characters
{

// ASCII related constants
inline CASTLE_CONSTEXPR uint8_t  ascii_control_min = 0x00U; // NUL
inline CASTLE_CONSTEXPR uint8_t  ascii_control_max = 0x20U; // SPACE
inline CASTLE_CONSTEXPR uint8_t  ascii_lower_min = 0x61U; // 'a'
inline CASTLE_CONSTEXPR uint8_t  ascii_lower_max = 0x7AU; // 'z'
inline CASTLE_CONSTEXPR uint8_t  ascii_upper_min = 0x41U; // 'A'
inline CASTLE_CONSTEXPR uint8_t  ascii_upper_max = 0x5AU; // 'Z'
inline CASTLE_CONSTEXPR uint8_t  ascii_digit_min = 0x30U; // '0'
inline CASTLE_CONSTEXPR uint8_t  ascii_digit_max = 0x39U; // '9'

// UTF-8 related constants
inline CASTLE_CONSTEXPR uint8_t  utf8_shift_6 = 6U;
inline CASTLE_CONSTEXPR uint8_t  utf8_shift_12 = 12U;
inline CASTLE_CONSTEXPR uint8_t  utf8_shift_18 = 18U;
inline CASTLE_CONSTEXPR uint8_t  utf8_ascii_max = 0x7FU;
inline CASTLE_CONSTEXPR uint8_t  utf8_lead2_min = 0xC2U;
inline CASTLE_CONSTEXPR uint8_t  utf8_lead2_max = 0xDFU;
inline CASTLE_CONSTEXPR uint8_t  utf8_lead3_min = 0xE0U;
inline CASTLE_CONSTEXPR uint8_t  utf8_lead3_max = 0xEFU;
inline CASTLE_CONSTEXPR uint8_t  utf8_lead4_min = 0xF0U;
inline CASTLE_CONSTEXPR uint8_t  utf8_lead4_max = 0xF4U;
inline CASTLE_CONSTEXPR uint8_t  utf8_two_byte_prefix = 0xC0U;
inline CASTLE_CONSTEXPR uint8_t  utf8_three_byte_prefix = 0xE0U;
inline CASTLE_CONSTEXPR uint8_t  utf8_four_byte_prefix = 0xF0U;
inline CASTLE_CONSTEXPR uint8_t  utf8_continuation_prefix = 0x80U;
inline CASTLE_CONSTEXPR uint8_t  utf8_continuation_mask = 0x3FU;
inline CASTLE_CONSTEXPR uint8_t  utf8_decode_mask_2 = 0x1FU;
inline CASTLE_CONSTEXPR uint8_t  utf8_decode_mask_3 = 0x0FU;
inline CASTLE_CONSTEXPR uint8_t  utf8_decode_mask_4 = 0x07U;
inline CASTLE_CONSTEXPR uint16_t utf8_one_byte_max = 0x007FU;
inline CASTLE_CONSTEXPR uint16_t utf8_two_byte_max = 0x07FFU;
inline CASTLE_CONSTEXPR uint32_t utf8_three_byte_max = 0xFFFFU;
inline CASTLE_CONSTEXPR uint32_t utf8_min_3_byte_codepoint = 0x800U;
inline CASTLE_CONSTEXPR uint32_t utf8_min_4_byte_codepoint = 0x10000U;
inline CASTLE_CONSTEXPR uint16_t utf16_surrogate_min = 0xD800U;
inline CASTLE_CONSTEXPR uint16_t utf16_surrogate_max = 0xDFFFU;

} // namespace characters

namespace serialization
{

// General serialization constants
inline CASTLE_CONSTEXPR size_type max_integer_str_len = 32U;
inline CASTLE_CONSTEXPR size_type max_floating_str_len = 48U;
inline CASTLE_CONSTEXPR uint32_t  unicode_max_codepoint = 0x10FFFFU;
// Overflow guard for the decimal exponent accumulated while parsing a
// floating-point literal (ini/json/xml all reject exponents beyond this).
inline CASTLE_CONSTEXPR int        max_floating_exponent = 1024;

// serialization::xml
inline CASTLE_CONSTEXPR uint32_t xml_tab = 0x09U;
inline CASTLE_CONSTEXPR uint32_t xml_line_feed = 0x0AU;
inline CASTLE_CONSTEXPR uint32_t xml_carriage_return = 0x0DU;
inline CASTLE_CONSTEXPR uint32_t xml_char_min = 0x20U;
inline CASTLE_CONSTEXPR uint32_t xml_bmp_first_max = 0xD7FFU;
inline CASTLE_CONSTEXPR uint32_t xml_bmp_second_min = 0xE000U;
inline CASTLE_CONSTEXPR uint32_t xml_bmp_second_max = 0xFFFDU;
inline CASTLE_CONSTEXPR uint32_t xml_supplementary_min = 0x10000U;
inline CASTLE_CONSTEXPR uint32_t xml_supplementary_max = 0x10FFFFU;

} // namespace serialization

} // namespace castle

#endif // CASTLE_CORE_CONSTANTS_HPP