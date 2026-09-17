// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file macros.hpp
 * @brief Preprocessor helpers for stringification, token concatenation, and bit masks.
 *
 * Use this header when code generation or register definitions need simple,
 * deterministic macros without pulling in the standard library. Every helper is
 * resolved entirely by the preprocessor and performs no runtime allocation or
 * stateful work.
 *
 * @code
 * auto mask = CASTLE_BIT(3);
 * auto text = CASTLE_STRING(device_id);
 * @endcode
 */
#ifndef CASTLE_UTILITY_MACROS_HPP
#define CASTLE_UTILITY_MACROS_HPP

/**
 * @brief Stringifies the raw variadic token sequence passed to it.
 */
#define CASTLE_STRINGIFY_HELPER(...) #__VA_ARGS__
/**
 * @brief Expands its arguments and then stringifies the result.
 */
#define CASTLE_STRINGIFY(...)        CASTLE_STRINGIFY_HELPER(__VA_ARGS__)

/**
 * @brief Concatenates two tokens without expanding the result further.
 */
#define CASTLE_CONCAT_HELPER(x, y)   x##y
/**
 * @brief Expands both arguments and concatenates them into one token.
 */
#define CASTLE_CONCAT(x, y)          CASTLE_CONCAT_HELPER(x, y)

/**
 * @brief Produces a narrow string literal from a token sequence.
 */
#define CASTLE_STRING(x)      CASTLE_CONCAT(,  CASTLE_STRINGIFY(x))
/**
 * @brief Produces a wide string literal from a token sequence.
 */
#define CASTLE_WIDE_STRING(x) CASTLE_CONCAT(L, CASTLE_STRINGIFY(x))
/**
 * @brief Produces a UTF-8 string literal from a token sequence.
 */
#define CASTLE_U8_STRING(x)   CASTLE_CONCAT(u8, CASTLE_STRINGIFY(x))
/**
 * @brief Produces a UTF-16 string literal from a token sequence.
 */
#define CASTLE_U16_STRING(x)  CASTLE_CONCAT(u,  CASTLE_STRINGIFY(x))
/**
 * @brief Produces a UTF-32 string literal from a token sequence.
 */
#define CASTLE_U32_STRING(x)  CASTLE_CONCAT(U,  CASTLE_STRINGIFY(x))

/**
 * @brief Returns a 32-bit mask with bit `n` set.
 */
#define CASTLE_BIT(n)   (1U << (n))
/**
 * @brief Returns a 64-bit mask with bit `n` set.
 */
#define CASTLE_BIT64(n) (1ULL << (n))

#endif // CASTLE_UTILITY_MACROS_HPP
