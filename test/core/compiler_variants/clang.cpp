#include <gtest/gtest.h>

#include "castle/core/compiler_variants/clang.h"

namespace
{

// clang.h forwards to gcc.h, so both identity macros and every GCC macro exist.
#if !defined(CASTLE_COMPILER_CLANG) || !defined(CASTLE_COMPILER_GCC)                      \
    || !defined(CASTLE_INLINE) || !defined(CASTLE_HOT) || !defined(CASTLE_COLD)           \
    || !defined(CASTLE_NOINLINE) || !defined(CASTLE_LIKELY) || !defined(CASTLE_UNLIKELY)  \
    || !defined(CASTLE_UNREACHABLE) || !defined(CASTLE_PACKED_ATTR) || !defined(CASTLE_RESTRICT)
#error "Clang compiler variant macros are not fully defined"
#endif

CASTLE_INLINE int clang_inline_add(int a, int b) { return a + b; }

TEST(CoreCompilerClangTest, IdentityMacrosDefined)
{
    EXPECT_EQ(CASTLE_COMPILER_CLANG, 1);
    EXPECT_EQ(CASTLE_COMPILER_GCC, 1);
}

TEST(CoreCompilerClangTest, ForwardedBranchHintsWork)
{
    EXPECT_TRUE(CASTLE_LIKELY(1 == 1));
    EXPECT_FALSE(CASTLE_UNLIKELY(1 == 2));
    EXPECT_EQ(clang_inline_add(20, 22), 42);
}

} // namespace
