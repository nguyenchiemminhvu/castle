#include <gtest/gtest.h>

#include "castle/core/compiler_variants/gcc.h"

namespace
{

// All GCC-variant macros must be present.
#if !defined(CASTLE_COMPILER_GCC) || !defined(CASTLE_INLINE) || !defined(CASTLE_HOT)      \
    || !defined(CASTLE_COLD) || !defined(CASTLE_NOINLINE) || !defined(CASTLE_LIKELY)      \
    || !defined(CASTLE_UNLIKELY) || !defined(CASTLE_UNREACHABLE)                          \
    || !defined(CASTLE_PACKED_ATTR) || !defined(CASTLE_RESTRICT)
#error "GCC compiler variant macros are not fully defined"
#endif

CASTLE_INLINE int gcc_inline_add(int a, int b) { return a + b; }
CASTLE_NOINLINE int gcc_noinline_sub(int a, int b) { return a - b; }
CASTLE_HOT int gcc_hot(int a) { return a + 1; }
CASTLE_COLD int gcc_cold(int a) { return a - 1; }

int gcc_restrict_sum(const int* CASTLE_RESTRICT a, const int* CASTLE_RESTRICT b)
{
    return *a + *b;
}

struct CASTLE_PACKED_ATTR GccPacked
{
    unsigned char a;
    int b;
};

TEST(CoreCompilerGccTest, CompilerIdentityMacro)
{
    EXPECT_EQ(CASTLE_COMPILER_GCC, 1);
}

TEST(CoreCompilerGccTest, BranchHintMacrosEvaluateOperand)
{
    EXPECT_TRUE(CASTLE_LIKELY(2 + 2 == 4));
    EXPECT_FALSE(CASTLE_UNLIKELY(2 + 2 == 5));
    EXPECT_EQ(CASTLE_LIKELY(1) ? 10 : 20, 10);
    EXPECT_EQ(CASTLE_UNLIKELY(0) ? 10 : 20, 20);
}

TEST(CoreCompilerGccTest, AttributedFunctionsBehaveNormally)
{
    EXPECT_EQ(gcc_inline_add(3, 4), 7);
    EXPECT_EQ(gcc_noinline_sub(10, 4), 6);
    EXPECT_EQ(gcc_hot(41), 42);
    EXPECT_EQ(gcc_cold(1), 0);

    const int x = 5;
    const int y = 9;
    EXPECT_EQ(gcc_restrict_sum(&x, &y), 14);
}

TEST(CoreCompilerGccTest, PackedAttributeRemovesPadding)
{
    EXPECT_EQ(sizeof(GccPacked), sizeof(unsigned char) + sizeof(int));
}

} // namespace
