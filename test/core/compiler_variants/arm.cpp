#include <gtest/gtest.h>

#include "castle/core/compiler_variants/arm.h"

namespace
{

// arm.h forwards to gcc.h, so both identity macros and every GCC macro exist.
#if !defined(CASTLE_COMPILER_ARM) || !defined(CASTLE_COMPILER_GCC)                        \
    || !defined(CASTLE_INLINE) || !defined(CASTLE_HOT) || !defined(CASTLE_COLD)           \
    || !defined(CASTLE_NOINLINE) || !defined(CASTLE_LIKELY) || !defined(CASTLE_UNLIKELY)  \
    || !defined(CASTLE_UNREACHABLE) || !defined(CASTLE_PACKED_ATTR) || !defined(CASTLE_RESTRICT)
#error "ARM compiler variant macros are not fully defined"
#endif

struct CASTLE_PACKED_ATTR ArmPacked
{
    unsigned char a;
    int b;
};

TEST(CoreCompilerArmTest, IdentityMacrosDefined)
{
    EXPECT_EQ(CASTLE_COMPILER_ARM, 1);
    EXPECT_EQ(CASTLE_COMPILER_GCC, 1);
}

TEST(CoreCompilerArmTest, ForwardedBranchHintsWork)
{
    EXPECT_TRUE(CASTLE_LIKELY(3 > 2));
    EXPECT_FALSE(CASTLE_UNLIKELY(2 > 3));
}

TEST(CoreCompilerArmTest, PackedAttributeRemovesPadding)
{
    EXPECT_EQ(sizeof(ArmPacked), sizeof(unsigned char) + sizeof(int));
}

} // namespace
