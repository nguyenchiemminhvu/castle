// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// GCC-style compiler mappings used by Castle's low-level portability layer.
// This header is selected directly for GCC and reused by Clang and ARM
// backends that support the same attributes and builtins. Use it indirectly
// through castle/core/compiler.hpp in normal code. The macros expose only
// compile-time annotations and intrinsics, keeping Castle deterministic and
// free of runtime allocation or exception machinery.

#ifndef CASTLE_CORE_COMPILER_VARIANTS_GCC_HPP
#define CASTLE_CORE_COMPILER_VARIANTS_GCC_HPP

/// @def CASTLE_COMPILER_GCC
/// @brief Indicates that the GCC-style backend definitions are available.
#define CASTLE_COMPILER_GCC  1

/// @def CASTLE_INLINE
/// @brief Requests aggressive inlining using GCC-style attributes.
#define CASTLE_INLINE        __attribute__((always_inline)) inline
/// @def CASTLE_HOT
/// @brief Marks a function as performance-critical.
#define CASTLE_HOT           __attribute__((hot))
/// @def CASTLE_COLD
/// @brief Marks a function as unlikely to execute.
#define CASTLE_COLD          __attribute__((cold))
/// @def CASTLE_NOINLINE
/// @brief Prevents a function from being inlined.
#define CASTLE_NOINLINE      __attribute__((noinline))

/// @def CASTLE_LIKELY
/// @brief Adds a branch-prediction hint for a condition that is usually true.
/// @param x Condition expression.
#define CASTLE_LIKELY(x)     __builtin_expect(!!(x), 1)
/// @def CASTLE_UNLIKELY
/// @brief Adds a branch-prediction hint for a condition that is usually false.
/// @param x Condition expression.
#define CASTLE_UNLIKELY(x)   __builtin_expect(!!(x), 0)

/// @def CASTLE_UNREACHABLE
/// @brief Marks a code path as unreachable to the optimizer.
#define CASTLE_UNREACHABLE() __builtin_unreachable()

/// @def CASTLE_PACKED_ATTR
/// @brief Applies packed layout to a declaration.
#define CASTLE_PACKED_ATTR   __attribute__((packed))
/// @def CASTLE_RESTRICT
/// @brief Expands to the GCC-style restrict qualifier.
#define CASTLE_RESTRICT      __restrict__

// Emit the lightest spin-wait hint available for the active architecture.
#if defined(__i386__) || defined(__x86_64__)
    /// @def CASTLE_CPU_RELAX
    /// @brief Emits an x86 pause instruction for busy-wait loops.
    #define CASTLE_CPU_RELAX() __builtin_ia32_pause()
#elif defined(__arm__) || defined(__aarch64__) || defined(__thumb__)
    /// @def CASTLE_CPU_RELAX
    /// @brief Emits an ARM yield hint for busy-wait loops.
    #define CASTLE_CPU_RELAX() __asm__ __volatile__("yield" ::: "memory")
#else
    /// @def CASTLE_CPU_RELAX
    /// @brief Emits a compiler barrier when no dedicated spin hint is known.
    #define CASTLE_CPU_RELAX() __asm__ __volatile__("" ::: "memory")
#endif

#endif // CASTLE_CORE_COMPILER_VARIANTS_GCC_HPP
