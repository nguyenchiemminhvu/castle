// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// ARM compiler-variant bridge for Castle's compiler abstraction.
// This header is selected by castle/core/compiler.hpp when an ARM compiler is
// detected. Use it only indirectly unless a build intentionally smoke-tests the
// backend header itself. ARM Compiler 6 reuses GCC/Clang-style attributes here,
// so the implementation forwards to the GCC variant and adds the ARM identity
// macro on top.

#ifndef CASTLE_CORE_COMPILER_VARIANTS_ARM_HPP
#define CASTLE_CORE_COMPILER_VARIANTS_ARM_HPP

/// @def CASTLE_COMPILER_ARM
/// @brief Indicates that the ARM compiler backend header was selected.
#define CASTLE_COMPILER_ARM  1

#include "castle/core/compiler_variants/gcc.hpp"

#endif // CASTLE_CORE_COMPILER_VARIANTS_ARM_HPP
