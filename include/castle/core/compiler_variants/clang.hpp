#ifndef CASTLE_CORE_COMPILER_VARIANTS_CLANG_HPP
#define CASTLE_CORE_COMPILER_VARIANTS_CLANG_HPP

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define CASTLE_COMPILER_CLANG 1

// Clang fully supports GCC attributes, so we forward directly to gcc.h
#include "castle/core/compiler_variants/gcc.hpp"

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif // CASTLE_CORE_COMPILER_VARIANTS_CLANG_HPP
