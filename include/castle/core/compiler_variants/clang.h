#ifndef CASTLE_CORE_COMPILER_VARIANTS_CLANG_H
#define CASTLE_CORE_COMPILER_VARIANTS_CLANG_H

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define CASTLE_COMPILER_CLANG 1

// Clang fully supports GCC attributes, so we forward directly to gcc.h
#include "castle/core/compiler_variants/gcc.h"

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif // CASTLE_CORE_COMPILER_VARIANTS_CLANG_H
