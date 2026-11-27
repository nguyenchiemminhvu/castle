// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// Conservative fallback compiler mappings for unsupported toolchains.
// This header is selected by castle/core/compiler.hpp when Castle does not
// recognize a dedicated compiler backend. Use it when bringing up a new
// freestanding toolchain or when a platform can only rely on standard C++
// keywords. The definitions avoid non-portable intrinsics and intentionally
// degrade optimization hints to no-ops.

#ifndef CASTLE_CORE_COMPILER_VARIANTS_DEFAULT_HPP
#define CASTLE_CORE_COMPILER_VARIANTS_DEFAULT_HPP

/// @def CASTLE_COMPILER_UNKNOWN
/// @brief Indicates that no dedicated Castle compiler backend matched.
#define CASTLE_COMPILER_UNKNOWN 1

/// @def CASTLE_INLINE
/// @brief Expands to a plain inline definition.
#define CASTLE_INLINE           inline
/// @def CASTLE_HOT
/// @brief Marks a hot function when supported; empty in the fallback backend.
#define CASTLE_HOT
/// @def CASTLE_COLD
/// @brief Marks a cold function when supported; empty in the fallback backend.
#define CASTLE_COLD
/// @def CASTLE_NOINLINE
/// @brief Prevents inlining when supported; empty in the fallback backend.
#define CASTLE_NOINLINE

/// @def CASTLE_LIKELY
/// @brief Wraps a condition that is expected to evaluate to true.
/// @param x Condition expression.
#define CASTLE_LIKELY(x)        (x)
/// @def CASTLE_UNLIKELY
/// @brief Wraps a condition that is expected to evaluate to false.
/// @param x Condition expression.
#define CASTLE_UNLIKELY(x)      (x)

/// @def CASTLE_UNREACHABLE
/// @brief Marks control flow as unreachable when supported; a no-op here.
#define CASTLE_UNREACHABLE()    ((void)0)

/// @def CASTLE_PACKED_ATTR
/// @brief Applies packed layout when supported; empty in the fallback backend.
#define CASTLE_PACKED_ATTR
/// @def CASTLE_RESTRICT
/// @brief Expands to a compiler restrict qualifier when supported; empty here.
#define CASTLE_RESTRICT

/// @def CASTLE_CPU_RELAX
/// @brief Emits a spin-wait hint when supported; a no-op in the fallback backend.
#define CASTLE_CPU_RELAX() ((void)0)

#endif // CASTLE_CORE_COMPILER_VARIANTS_DEFAULT_HPP
