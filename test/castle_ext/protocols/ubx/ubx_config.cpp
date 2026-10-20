#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/ubx_config.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::config;

// ---------------------------------------------------------------------------
// config_value
// ---------------------------------------------------------------------------

TEST(UbxConfigValue, DefaultIsZero)
{
    config_value value;
    EXPECT_EQ(value.as_u64(), 0U);
    EXPECT_FALSE(value.as_bool());
}

TEST(UbxConfigValue, BoolConstruction)
{
    EXPECT_TRUE(config_value(true).as_bool());
    EXPECT_FALSE(config_value(false).as_bool());
    EXPECT_EQ(config_value(true).as_u64(), 1ULL);
    EXPECT_EQ(config_value(false).as_u64(), 0ULL);
}

TEST(UbxConfigValue, UnsignedConstructionAndAccessors)
{
    EXPECT_EQ(config_value(static_cast<uint8_t>(0xABU)).as_u8(), 0xABU);
    EXPECT_EQ(config_value(static_cast<uint16_t>(0x1234U)).as_u16(), 0x1234U);
    EXPECT_EQ(config_value(static_cast<uint32_t>(0x12345678U)).as_u32(), 0x12345678U);
    EXPECT_EQ(config_value(static_cast<uint64_t>(0x0102030405060708ULL)).as_u64(), 0x0102030405060708ULL);
}

TEST(UbxConfigValue, SignedValuesKeepTwosComplementBitPattern)
{
    EXPECT_EQ(config_value(static_cast<int8_t>(-1)).as_i8(), static_cast<int8_t>(-1));
    EXPECT_EQ(config_value(static_cast<int8_t>(-1)).as_u64(), 0xFFULL);
    EXPECT_EQ(config_value(static_cast<int8_t>(5)).as_i8(), static_cast<int8_t>(5));

    EXPECT_EQ(config_value(static_cast<int16_t>(-1234)).as_i16(), static_cast<int16_t>(-1234));
    EXPECT_EQ(config_value(static_cast<int16_t>(-1)).as_u64(), 0xFFFFULL);

    const config_value value32(static_cast<int32_t>(-2));
    EXPECT_EQ(value32.as_i32(), -2);
    EXPECT_EQ(value32.as_u32(), 0xFFFFFFFEU);
    EXPECT_EQ(value32.as_u64(), 0xFFFFFFFEULL); // not sign-extended into the upper 32 bits

    EXPECT_EQ(config_value(static_cast<int64_t>(-1)).as_i64(), static_cast<int64_t>(-1));
    EXPECT_EQ(config_value(static_cast<int64_t>(-1)).as_u64(), 0xFFFFFFFFFFFFFFFFULL);
}

TEST(UbxConfigValue, NarrowAccessorsTruncateWideValues)
{
    const config_value value(static_cast<uint64_t>(0x1122334455667788ULL));
    EXPECT_EQ(value.as_u8(), 0x88U);
    EXPECT_EQ(value.as_u16(), 0x7788U);
    EXPECT_EQ(value.as_u32(), 0x55667788U);
    EXPECT_TRUE(value.as_bool());
}

TEST(UbxConfigValue, IsUsableInConstantExpressions)
{
    static CASTLE_CONSTEXPR config_value value(static_cast<uint16_t>(1000U));
    static_assert(value.as_u16() == 1000U, "config_value must be constexpr-constructible");
    EXPECT_EQ(value.as_u16(), 1000U);
}

TEST(UbxConfigValue, EqualityOperators)
{
    config_value a(static_cast<uint32_t>(42U));
    config_value b(static_cast<uint32_t>(42U));
    config_value c(static_cast<uint32_t>(43U));

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);

    // Equality compares the raw bit pattern, regardless of the constructing type.
    EXPECT_TRUE(config_value(true) == config_value(static_cast<uint8_t>(1U)));
}

// ---------------------------------------------------------------------------
// config_entry
// ---------------------------------------------------------------------------

TEST(UbxConfigEntry, DefaultAndAggregateConstruction)
{
    config_entry empty{};
    EXPECT_EQ(empty.key_id, 0U);
    EXPECT_EQ(empty.value.as_u64(), 0U);

    config_entry rate{0x30210001U, config_value(static_cast<uint16_t>(1000U))};
    EXPECT_EQ(rate.key_id, 0x30210001U);
    EXPECT_EQ(rate.value.as_u16(), 1000U);
    EXPECT_EQ(value_byte_size(rate.key_id), 2U); // width follows the key, not the value type
}

// ---------------------------------------------------------------------------
// config_layer
// ---------------------------------------------------------------------------

TEST(UbxConfigLayer, OperatorOrCombinesBitFlags)
{
    EXPECT_EQ(static_cast<uint8_t>(config_layer::ram), 0x01U);
    EXPECT_EQ(static_cast<uint8_t>(config_layer::bbr), 0x02U);
    EXPECT_EQ(static_cast<uint8_t>(config_layer::flash), 0x04U);
    EXPECT_EQ(config_layer::ram | config_layer::bbr, 0x03U);
    EXPECT_EQ(config_layer::ram | config_layer::flash, 0x05U);
    EXPECT_EQ(config_layer::bbr | config_layer::flash, 0x06U);
    EXPECT_EQ(config_layer::ram | config_layer::ram, 0x01U);
}

TEST(UbxConfigLayer, ValgetSourceLayersAreSelectorsNotBits)
{
    EXPECT_EQ(UBX_CFG_VALGET_LAYER_RAM, 0U);
    EXPECT_EQ(UBX_CFG_VALGET_LAYER_BBR, 1U);
    EXPECT_EQ(UBX_CFG_VALGET_LAYER_FLASH, 2U);
    EXPECT_EQ(UBX_CFG_VALGET_LAYER_DEFAULT, 7U);
}

} // namespace
