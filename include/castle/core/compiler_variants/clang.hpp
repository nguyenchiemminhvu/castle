// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// Clang compiler-variant bridge for Castle's compiler abstraction.
// This header is selected by castle/core/compiler.hpp when Clang is detected.
// Use it indirectly through the public compiler wrapper unless you are
// validating backend inclusion. Clang shares GCC-style attributes and builtins
// for the facilities Castle uses here, so the implementation forwards to the
// GCC variant and adds a Clang identity macro.

#ifndef CASTLE_CORE_COMPILER_VARIANTS_CLANG_HPP
#define CASTLE_CORE_COMPILER_VARIANTS_CLANG_HPP

/// @def CASTLE_COMPILER_CLANG
/// @brief Indicates that the Clang compiler backend header was selected.
#define CASTLE_COMPILER_CLANG 1

#include "castle/core/compiler_variants/gcc.hpp"

#endif // CASTLE_CORE_COMPILER_VARIANTS_CLANG_HPP
