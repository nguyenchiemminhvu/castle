#include <gtest/gtest.h>

#include "castle/units/mass.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

TEST(UnitsMass, UnitDefinitions)
{
    static_assert(is_same<cu::kilogram_unit::dimension, cu::dim::mass>::value, "dimension");
    static_assert(cu::kilogram_unit::ratio::num == 1 && cu::kilogram_unit::ratio::den == 1, "kg coherent");
    static_assert(cu::gram_unit::ratio::num == 1 && cu::gram_unit::ratio::den == 1000, "g");
    static_assert(cu::milligram_unit::ratio::den == 1000000, "mg");
    static_assert(cu::tonne_unit::ratio::num == 1000, "t");
    static_assert(cu::pound_unit::ratio::num == 45359237 && cu::pound_unit::ratio::den == 100000000, "lb");
    static_assert(cu::ounce_unit::ratio::num == 45359237 && cu::ounce_unit::ratio::den == 1600000000, "oz");
    static_assert(is_same<cu::gram_unit::dimension, cu::dim::mass>::value, "g dimension");
    EXPECT_EQ(cu::ounce_unit::ratio::den, 16 * cu::pound_unit::ratio::den);
}

TEST(UnitsMass, ExactConversions)
{
    EXPECT_EQ(cu::grams<int>(cu::kilograms<int>(3)).value(), 3000);
    EXPECT_EQ(cu::milligrams<int64_t>(cu::grams<int64_t>(2)).value(), 2000);
    EXPECT_EQ(cu::kilograms<int>(cu::tonnes<int>(2)).value(), 2000);
    EXPECT_EQ(cu::grams<int>(cu::tonnes<int>(1)).value(), 1000000);
    EXPECT_EQ(cu::ounces<int>(cu::pounds<int>(2)).value(), 32);
    EXPECT_EQ(cu::kilograms<int>(cu::grams<int>(1999)).value(), 1);

    const cu::kilograms<double> pound = cu::pounds<double>(1.0);
    EXPECT_DOUBLE_EQ(pound.value(), 0.45359237);
    const cu::kilograms<double> ounce = cu::ounces<double>(1.0);
    EXPECT_DOUBLE_EQ(ounce.value(), 0.028349523125);
    const cu::kilograms<double> gram = cu::grams<double>(500.0);
    EXPECT_DOUBLE_EQ(gram.value(), 0.5);
    const cu::kilograms<double> milligram = cu::milligrams<double>(250000.0);
    EXPECT_DOUBLE_EQ(milligram.value(), 0.25);
    const cu::pounds<double> back = cu::kilograms<double>(0.45359237);
    EXPECT_DOUBLE_EQ(back.value(), 1.0);
}

TEST(UnitsMass, Literals)
{
    static_assert(is_same<decltype(1.5_kg), cu::kilograms<double>>::value, "kg");
    static_assert(is_same<decltype(15_kg), cu::kilograms<int64_t>>::value, "kg int");
    static_assert(is_same<decltype(1.5_g), cu::grams<double>>::value, "g");
    static_assert(is_same<decltype(15_g), cu::grams<int64_t>>::value, "g int");
    static_assert(is_same<decltype(1.5_mg), cu::milligrams<double>>::value, "mg");
    static_assert(is_same<decltype(15_mg), cu::milligrams<int64_t>>::value, "mg int");
    static_assert(is_same<decltype(1.5_t), cu::tonnes<double>>::value, "t");
    static_assert(is_same<decltype(15_t), cu::tonnes<int64_t>>::value, "t int");
    static_assert(is_same<decltype(1.5_lb), cu::pounds<double>>::value, "lb");
    static_assert(is_same<decltype(15_lb), cu::pounds<int64_t>>::value, "lb int");
    static_assert(is_same<decltype(1.5_oz), cu::ounces<double>>::value, "oz");
    static_assert(is_same<decltype(15_oz), cu::ounces<int64_t>>::value, "oz int");
    EXPECT_DOUBLE_EQ((1.5_kg).value(), 1.5);
    EXPECT_EQ((15_kg).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_g).value(), 1.5);
    EXPECT_EQ((15_g).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_mg).value(), 1.5);
    EXPECT_EQ((15_mg).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_t).value(), 1.5);
    EXPECT_EQ((15_t).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_lb).value(), 1.5);
    EXPECT_EQ((15_lb).value(), 15);
    EXPECT_DOUBLE_EQ((1.5_oz).value(), 1.5);
    EXPECT_EQ((15_oz).value(), 15);
}

TEST(UnitsMass, DerivedArithmetic)
{
    const cu::kilograms<double> payload = 1.5_kg + 250.0_g;
    EXPECT_DOUBLE_EQ(payload.value(), 1.75);
    EXPECT_TRUE(1_t == 1000_kg);
    EXPECT_TRUE(16_oz == 1_lb);
    EXPECT_TRUE(1_kg > 2_lb);
    EXPECT_TRUE(500_mg < 1_g);
    const auto doubled = 2.0_kg * 3.0_kg;
    static_assert(decltype(doubled)::dimension::mass == 2, "mass squared");
    EXPECT_DOUBLE_EQ(doubled.value(), 6.0);
    const cu::grams<int64_t> diff = 2_kg - 500_g;
    EXPECT_EQ(diff.value(), 1500);
}

} // namespace
