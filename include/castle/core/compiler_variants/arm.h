#ifndef CASTLE_CORE_COMPILER_VARIANTS_ARM_H
#define CASTLE_CORE_COMPILER_VARIANTS_ARM_H

#define CASTLE_COMPILER_ARM  1

// ARM Compiler 6 is Clang/GCC based, so we reuse the definitions
#include "castle/core/compiler_variants/gcc.h"

// If we eventually need ARM-specific overrides,
// we can safely add or redefine them right here.

#endif // CASTLE_CORE_COMPILER_VARIANTS_ARM_H
