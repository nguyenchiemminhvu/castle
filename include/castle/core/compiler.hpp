// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// Castle compiler abstraction and annotation macros.
// Include this header when low-level Castle code needs a single set of
// compiler-detection switches, attributes, keyword wrappers, or branch-hint
// helpers without depending on a specific toolchain spelling. The header only
// selects and forwards compile-time facilities; it performs no allocation,
// introduces no runtime state, and is intended to be included transitively by
// most Castle headers.

#ifndef CASTLE_CORE_COMPILER_HPP
#define CASTLE_CORE_COMPILER_HPP

// Select the backend attribute mapping that matches the active compiler.
#if defined(__clang__)
    #include "compiler_variants/clang.hpp"
#elif defined(__ARMCC_VERSION) || defined(__CC_ARM)
    #include "compiler_variants/arm.hpp"
#elif defined(__GNUC__)
    #include "compiler_variants/gcc.hpp"
#else
    #include "compiler_variants/default.hpp"
#endif

/// @def CASTLE_UNUSED
/// @brief Marks an entity as intentionally unused.
#define CASTLE_UNUSED            [[maybe_unused]]
/// @def CASTLE_NODISCARD
/// @brief Requests a diagnostic when a return value is ignored.
#define CASTLE_NODISCARD         [[nodiscard]]
/// @def CASTLE_FALL_THROUGH
/// @brief Marks an intentional fallthrough between switch labels.
#define CASTLE_FALL_THROUGH      [[fallthrough]]
/// @def CASTLE_DEPRECATED
/// @brief Marks a declaration as deprecated.
#define CASTLE_DEPRECATED        [[deprecated]]
/// @def CASTLE_DEPRECATED_MSG
/// @brief Marks a declaration as deprecated with a custom message.
/// @param x Deprecation message string literal.
#define CASTLE_DEPRECATED_MSG(x) [[deprecated(x)]]

/// @def CASTLE_VOLATILE
/// @brief Expands to the C++ volatile qualifier.
#define CASTLE_VOLATILE          volatile
/// @def CASTLE_MUTABLE
/// @brief Expands to the C++ mutable keyword.
#define CASTLE_MUTABLE           mutable
/// @def CASTLE_CONST
/// @brief Expands to the C++ const qualifier.
#define CASTLE_CONST             const
/// @def CASTLE_CONSTEXPR
/// @brief Expands to the C++ constexpr keyword.
#define CASTLE_CONSTEXPR         constexpr
/// @def CASTLE_IF_CONSTEXPR
/// @brief Expands to the C++ if constexpr statement form.
#define CASTLE_IF_CONSTEXPR      if constexpr
/// @def CASTLE_NOEXCEPT
/// @brief Expands to the C++ noexcept specifier.
#define CASTLE_NOEXCEPT          noexcept
/// @def CASTLE_DEFAULT
/// @brief Expands to an explicitly defaulted special member definition.
#define CASTLE_DEFAULT           = default
/// @def CASTLE_DELETE
/// @brief Expands to an explicitly deleted special member definition.
#define CASTLE_DELETE            = delete
/// @def CASTLE_OVERRIDE
/// @brief Expands to the C++ override specifier.
#define CASTLE_OVERRIDE          override
/// @def CASTLE_FINAL
/// @brief Expands to the C++ final specifier.
#define CASTLE_FINAL             final
/// @def CASTLE_VIRTUAL
/// @brief Expands to the C++ virtual keyword.
#define CASTLE_VIRTUAL           virtual
/// @def CASTLE_MOVE
/// @brief Refers to castle::move when that utility is available.
#define CASTLE_MOVE              castle::move
/// @def CASTLE_FORWARD
/// @brief Refers to castle::forward when that utility is available.
#define CASTLE_FORWARD           castle::forward
/// @def CASTLE_STD
/// @brief Refers to the std namespace without spelling it directly.
#define CASTLE_STD               std

#endif // CASTLE_CORE_COMPILER_HPP
