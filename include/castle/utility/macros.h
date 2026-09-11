#ifndef CASTLE_UTILITY_MACROS_H
#define CASTLE_UTILITY_MACROS_H

// =====================================================
// Preprocessor helpers
// =====================================================

#define CASTLE_STRINGIFY_HELPER(...) #__VA_ARGS__
#define CASTLE_STRINGIFY(...)        CASTLE_STRINGIFY_HELPER(__VA_ARGS__)

#define CASTLE_CONCAT_HELPER(x, y)   x##y
#define CASTLE_CONCAT(x, y)          CASTLE_CONCAT_HELPER(x, y)

// =====================================================
// String literal helpers
// =====================================================

#define CASTLE_STRING(x)      CASTLE_CONCAT(,  CASTLE_STRINGIFY(x))
#define CASTLE_WIDE_STRING(x) CASTLE_CONCAT(L, CASTLE_STRINGIFY(x))
#define CASTLE_U8_STRING(x)   CASTLE_CONCAT(u8, CASTLE_STRINGIFY(x))
#define CASTLE_U16_STRING(x)  CASTLE_CONCAT(u,  CASTLE_STRINGIFY(x))
#define CASTLE_U32_STRING(x)  CASTLE_CONCAT(U,  CASTLE_STRINGIFY(x))

// =====================================================
// Bit helpers
// =====================================================

#define CASTLE_BIT(n)   (1U << (n))
#define CASTLE_BIT64(n) (1ULL << (n))

#endif // CASTLE_UTILITY_MACROS_H