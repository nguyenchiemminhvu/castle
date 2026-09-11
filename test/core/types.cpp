#include <gtest/gtest.h>

#include <cstddef>
#include <type_traits>

#include "castle/core/types.h"

namespace
{

TEST(CoreTypesTest, SizeTypeAliasesStdSizeT)
{
    static_assert(std::is_same<castle::size_type, std::size_t>::value,
                  "size_type must alias std::size_t");
    EXPECT_EQ(sizeof(castle::size_type), sizeof(std::size_t));
}

TEST(CoreTypesTest, DifferenceTypeAliasesPtrdiffT)
{
    static_assert(std::is_same<castle::difference_type, std::ptrdiff_t>::value,
                  "difference_type must alias std::ptrdiff_t");
    EXPECT_EQ(sizeof(castle::difference_type), sizeof(std::ptrdiff_t));
}

TEST(CoreTypesTest, SizeTypeIsUnsigned)
{
    static_assert(std::is_unsigned<castle::size_type>::value, "size_type must be unsigned");
    // Unsigned wrap-around: 0 - 1 is the maximum value, which is > 0.
    const castle::size_type zero = 0;
    EXPECT_GT(static_cast<castle::size_type>(zero - 1), zero);
}

TEST(CoreTypesTest, DifferenceTypeIsSigned)
{
    static_assert(std::is_signed<castle::difference_type>::value,
                  "difference_type must be signed");
    EXPECT_LT(static_cast<castle::difference_type>(-1), static_cast<castle::difference_type>(0));
}

TEST(CoreTypesTest, PointerSizedOnThisPlatform)
{
    EXPECT_EQ(sizeof(castle::size_type), sizeof(void*));
    EXPECT_EQ(sizeof(castle::difference_type), sizeof(void*));
}

} // namespace
