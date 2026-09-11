#include <gtest/gtest.h>

// Include the fallback variant directly. It is NOT the active variant on this
// platform, but the header stands alone and its macros can be exercised in
// isolation because gcc.h is never pulled in here.
#include "castle/core/compiler_variants/default.h"

namespace
{

#if !defined(CASTLE_COMPILER_UNKNOWN) || !defined(CASTLE_INLINE) || !defined(CASTLE_HOT)  \
    || !defined(CASTLE_COLD) || !defined(CASTLE_NOINLINE) || !defined(CASTLE_LIKELY)      \
    || !defined(CASTLE_UNLIKELY) || !defined(CASTLE_UNREACHABLE)                          \
    || !defined(CASTLE_PACKED_ATTR) || !defined(CASTLE_RESTRICT)
#error "Default compiler variant macros are not fully defined"
#endif

CASTLE_INLINE int default_inline_add(int a, int b) { return a + b; }

TEST(CoreCompilerDefaultTest, IdentityMacroDefined)
{
    EXPECT_EQ(CASTLE_COMPILER_UNKNOWN, 1);
}

TEST(CoreCompilerDefaultTest, BranchHintsArePassThrough)
{
    // In the fallback variant CASTLE_LIKELY/UNLIKELY simply forward the operand.
    EXPECT_EQ(CASTLE_LIKELY(7), 7);
    EXPECT_EQ(CASTLE_UNLIKELY(0), 0);
    EXPECT_TRUE(CASTLE_LIKELY(1 == 1));
    EXPECT_FALSE(CASTLE_UNLIKELY(1 == 2));
}

TEST(CoreCompilerDefaultTest, InlineMacroCompilesAsPlainInline)
{
    EXPECT_EQ(default_inline_add(19, 23), 42);
}

TEST(CoreCompilerDefaultTest, UnreachableMacroIsSafeStatement)
{
    // The fallback expands CASTLE_UNREACHABLE() to a no-op, so executing it is
    // well-defined (unlike the __builtin_unreachable() variant).
    CASTLE_UNREACHABLE();
    SUCCEED();
}

} // namespace
