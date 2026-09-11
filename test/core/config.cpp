#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

#include "castle/core/config.h"

namespace
{

// Endianness selection macros must be defined and mutually exclusive.
#if !defined(CASTLE_ENDIAN_NATIVE) || !defined(CASTLE_IS_LITTLE_ENDIAN)                  \
    || !defined(CASTLE_IS_BIG_ENDIAN) || !defined(CASTLE_HAS_CONSTEXPR_ENDIANNESS)
#error "CASTLE endianness macros are not defined"
#endif

TEST(CoreConfigTest, EndianConstantMacrosHaveCanonicalValues)
{
    EXPECT_EQ(CASTLE_ENDIAN_LITTLE, 0);
    EXPECT_EQ(CASTLE_ENDIAN_BIG, 1);
    EXPECT_EQ(CASTLE_HAS_CONSTEXPR_ENDIANNESS, 1);
}

TEST(CoreConfigTest, NativeEndianIsLittleOnThisPlatform)
{
    EXPECT_EQ(CASTLE_ENDIAN_NATIVE, CASTLE_ENDIAN_LITTLE);
    EXPECT_TRUE(CASTLE_IS_LITTLE_ENDIAN);
    EXPECT_FALSE(CASTLE_IS_BIG_ENDIAN);
}

TEST(CoreConfigTest, EndianEnumMirrorsMacros)
{
    static_assert(static_cast<unsigned char>(castle::endian::little) == CASTLE_ENDIAN_LITTLE,
                  "little enumerator mismatch");
    static_assert(static_cast<unsigned char>(castle::endian::big) == CASTLE_ENDIAN_BIG,
                  "big enumerator mismatch");
    static_assert(castle::endian::native == castle::endian::little,
                  "native must equal little on this platform");
    static_assert(castle::native_endian == castle::endian::native, "native_endian mismatch");

    // The enum's underlying type is unsigned char.
    static_assert(std::is_same<std::underlying_type<castle::endian>::type,
                               unsigned char>::value,
                  "endian underlying type should be unsigned char");
}

TEST(CoreConfigTest, EndianBoolConstantsMatch)
{
    static_assert(castle::is_little_endian == true, "is_little_endian mismatch");
    static_assert(castle::is_big_endian == false, "is_big_endian mismatch");
    EXPECT_TRUE(castle::is_little_endian);
    EXPECT_FALSE(castle::is_big_endian);
}

TEST(CoreConfigTest, RuntimeByteOrderAgreesWithConstants)
{
    const uint32_t value = 0x01020304u;
    const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
    if (castle::is_little_endian)
    {
        EXPECT_EQ(bytes[0], 0x04u);
        EXPECT_EQ(bytes[3], 0x01u);
    }
    else
    {
        EXPECT_EQ(bytes[0], 0x01u);
        EXPECT_EQ(bytes[3], 0x04u);
    }
}

TEST(CoreConfigTest, PlatformSizeMacrosAndConstants)
{
    EXPECT_EQ(CASTLE_PLATFORM_16BIT, sizeof(void*) == 2);
    EXPECT_EQ(CASTLE_PLATFORM_32BIT, sizeof(void*) == 4);
    EXPECT_EQ(CASTLE_PLATFORM_64BIT, sizeof(void*) == 8);

    EXPECT_EQ(castle::platform_16bit, sizeof(void*) == 2);
    EXPECT_EQ(castle::platform_32bit, sizeof(void*) == 4);
    EXPECT_EQ(castle::platform_64bit, sizeof(void*) == 8);

    // Exactly one platform-size flag is true.
    EXPECT_EQ(castle::platform_16bit + castle::platform_32bit + castle::platform_64bit, 1);
}

TEST(CoreConfigTest, PlatformIs64BitHere)
{
    EXPECT_TRUE(castle::platform_64bit);
    EXPECT_FALSE(castle::platform_32bit);
    EXPECT_FALSE(castle::platform_16bit);
}

TEST(CoreConfigTest, InplaceStorageConstantsAreConsistent)
{
    static_assert(castle::inplace_function_storage_words == 8U, "word count changed");
    static_assert(castle::inplace_storage_reserved
                      == castle::inplace_function_storage_words * sizeof(void*),
                  "reserved bytes derivation");
    static_assert(castle::inplace_alignment_default == alignof(castle::max_align_t),
                  "default alignment derivation");

    EXPECT_EQ(castle::inplace_function_storage_words, 8U);
    EXPECT_EQ(castle::inplace_storage_reserved, 8U * sizeof(void*));
    EXPECT_EQ(castle::inplace_alignment_default, alignof(castle::max_align_t));

    // Types are the library's size_type.
    static_assert(std::is_same<decltype(castle::inplace_storage_reserved), const castle::size_type>::value,
                  "inplace_storage_reserved should be size_type");
}

} // namespace
