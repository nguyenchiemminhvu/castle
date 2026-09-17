#include <gtest/gtest.h>

#include "castle/memory/alignment.h"

#include <cstdint>

namespace
{

TEST(AlignmentTest, IsAlignedRuntimeAcceptsAlignedPointer)
{
    alignas(64) unsigned char buffer[64] = {};
    EXPECT_TRUE(castle::memory::is_aligned(&buffer[0], 1U));
    EXPECT_TRUE(castle::memory::is_aligned(&buffer[0], 2U));
    EXPECT_TRUE(castle::memory::is_aligned(&buffer[0], 16U));
    EXPECT_TRUE(castle::memory::is_aligned(&buffer[0], 64U));
}

TEST(AlignmentTest, IsAlignedRuntimeRejectsMisalignedPointer)
{
    alignas(64) unsigned char buffer[64] = {};
    EXPECT_FALSE(castle::memory::is_aligned(&buffer[1], 2U));
    EXPECT_FALSE(castle::memory::is_aligned(&buffer[1], 16U));
}

TEST(AlignmentTest, IsAlignedRuntimeRejectsNonPowerOfTwoAlignment)
{
    alignas(64) unsigned char buffer[64] = {};
    EXPECT_FALSE(castle::memory::is_aligned(&buffer[0], 0U));
    EXPECT_FALSE(castle::memory::is_aligned(&buffer[0], 3U));
    EXPECT_FALSE(castle::memory::is_aligned(&buffer[0], 6U));
}

TEST(AlignmentTest, IsAlignedTemplateByAlignmentValue)
{
    alignas(32) unsigned char buffer[64] = {};
    EXPECT_TRUE(castle::memory::is_aligned<16U>(&buffer[0]));
    EXPECT_TRUE(castle::memory::is_aligned<32U>(&buffer[0]));
    EXPECT_FALSE(castle::memory::is_aligned<16U>(&buffer[1]));
}

TEST(AlignmentTest, IsAlignedTemplateByType)
{
    alignas(alignof(double)) unsigned char buffer[32] = {};
    EXPECT_TRUE(castle::memory::is_aligned<double>(&buffer[0]));
    EXPECT_TRUE(castle::memory::is_aligned<int>(&buffer[0]));
    EXPECT_TRUE(castle::memory::is_aligned<char>(&buffer[1]));
}

TEST(AlignmentTest, TypeWithAlignmentHasRequestedAlignment)
{
    static_assert(alignof(castle::memory::type_with_alignment_t<1U>) == 1U, "align 1");
    static_assert(alignof(castle::memory::type_with_alignment_t<2U>) == 2U, "align 2");
    static_assert(alignof(castle::memory::type_with_alignment_t<8U>) == 8U, "align 8");
    static_assert(alignof(castle::memory::type_with_alignment_t<16U>) == 16U, "align 16");

    // Public top-level alias mirrors the nested one.
    static_assert(alignof(castle::type_with_alignment_t<4U>) == 4U, "top-level alias");
    SUCCEED();
}

} // namespace
