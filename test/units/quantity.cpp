#include <gtest/gtest.h>

#include "castle/units/quantity.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
namespace cm = castle::math;

// Local units keep this test independent of the quantity headers.
using length_unit = cu::unit<cu::dim::length>;
using km_unit = cu::scaled_unit_t<length_unit, cu::kilo>;
using mm_unit = cu::scaled_unit_t<length_unit, cu::milli>;
using ft_unit = cu::unit<cu::dim::length, cm::ratio<3048, 10000>>;
using rad_unit = cu::unit<cu::dim::angle>;
using deg_unit = cu::unit<cu::dim::angle, cm::ratio<1, 180>, 1>;
using kelvin_unit = cu::unit<cu::dim::temperature>;
using celsius_unit = cu::unit<cu::dim::temperature, cm::ratio<1>, 0, cm::ratio<27315, 100>>;
using fahrenheit_unit = cu::unit<cu::dim::temperature, cm::ratio<5, 9>, 0, cm::ratio<45967, 180>>;
using velocity_dim = cu::dimension_divide_t<cu::dim::length, cu::dim::time>;
using mps_unit = cu::unit<velocity_dim>;
using hz_unit = cu::unit<cu::dimension_inverse_t<cu::dim::time>>;
struct other_kind
{
};
using kinded_rad_unit = cu::unit<cu::dim::angle, cm::ratio<1>, 0, cm::ratio<0>, other_kind>;

template <typename Rep> using meters = cu::quantity<length_unit, Rep>;
template <typename Rep> using kilometers = cu::quantity<km_unit, Rep>;
template <typename Rep> using millimeters = cu::quantity<mm_unit, Rep>;
template <typename Rep> using feet = cu::quantity<ft_unit, Rep>;
template <typename Rep> using radians = cu::quantity<rad_unit, Rep>;
template <typename Rep> using degrees = cu::quantity<deg_unit, Rep>;
template <typename Rep> using kelvin = cu::quantity<kelvin_unit, Rep>;
template <typename Rep> using celsius = cu::quantity<celsius_unit, Rep>;
template <typename Rep> using fahrenheit = cu::quantity<fahrenheit_unit, Rep>;
template <typename Rep> using mps = cu::quantity<mps_unit, Rep>;
template <typename Rep> using hertz = cu::quantity<hz_unit, Rep>;
template <typename Rep> using kinded_radians = cu::quantity<kinded_rad_unit, Rep>;

using cu::detail::is_lossless_conversion;

template <typename A, typename B, typename = void>
struct can_add : castle::meta::false_type
{
};

template <typename A, typename B>
struct can_add<A, B, castle::meta::void_t<decltype(castle::meta::declval<A>() + castle::meta::declval<B>())>>
    : castle::meta::true_type
{
};

template <typename A, typename B, typename = void>
struct can_compare : castle::meta::false_type
{
};

template <typename A, typename B>
struct can_compare<A, B, castle::meta::void_t<decltype(castle::meta::declval<A>() < castle::meta::declval<B>())>>
    : castle::meta::true_type
{
};

TEST(UnitsQuantity, RejectsMixingIncompatibleQuantities)
{
    static_assert(can_add<meters<double>, kilometers<int>>::value, "same dimension adds");
    static_assert(!can_add<meters<double>, radians<double>>::value, "length + angle");
    static_assert(!can_add<radians<double>, kinded_radians<double>>::value, "different kind");
    static_assert(!can_add<meters<double>, double>::value, "quantity + raw scalar");
    static_assert(can_compare<celsius<int>, kelvin<int>>::value, "temperatures compare");
    static_assert(!can_compare<meters<int>, mps<int>>::value, "length < velocity");
    EXPECT_FALSE((can_add<meters<double>, radians<double>>::value));
    EXPECT_TRUE((can_add<meters<double>, kilometers<int>>::value));
}
using castle::meta::is_same;

TEST(UnitsDimension, ArithmeticOnExponents)
{
    using area = cu::dimension_multiply_t<cu::dim::length, cu::dim::length>;
    static_assert(area::length == 2 && area::time == 0, "area");
    static_assert(velocity_dim::length == 1 && velocity_dim::time == -1, "velocity");
    using inv = cu::dimension_inverse_t<velocity_dim>;
    static_assert(inv::length == -1 && inv::time == 1, "inverse");
    static_assert(is_same<cu::dimension_divide_t<area, cu::dim::length>, cu::dim::length>::value, "divide");
    static_assert(cu::dim::angle::angle == 1 && cu::dim::mass::mass == 1, "base");
    static_assert(cu::dim::current::current == 1 && cu::dim::temperature::temperature == 1, "base");
    static_assert(cu::dim::amount::amount == 1 && cu::dim::luminous_intensity::luminous == 1, "base");
    EXPECT_EQ(area::length, 2);
    EXPECT_EQ(velocity_dim::time, -1);
}

TEST(UnitsUnit, CanonicalisationAndHelpers)
{
    // Unreduced ratios produce the same unit type.
    static_assert(is_same<cu::unit<cu::dim::length, cm::ratio<1000, 1000000>>, mm_unit>::value, "reduced");
    static_assert(is_same<km_unit::ratio, cm::ratio<1000>>::value, "scaled");
    static_assert(is_same<cu::coherent_unit_t<deg_unit>, rad_unit>::value, "coherent");
    static_assert(is_same<cu::coherent_unit_t<celsius_unit>, kelvin_unit>::value, "coherent offset");
    static_assert(deg_unit::pi_exponent == 1, "pi");
    static_assert(celsius_unit::offset::num == 5463 && celsius_unit::offset::den == 20, "offset");
    static_assert(cu::is_quantity<meters<int>>::value, "is_quantity");
    static_assert(!cu::is_quantity<int>::value, "is_quantity");
    static_assert(!cu::is_quantity<castle::chrono::seconds>::value, "is_quantity");
    EXPECT_EQ(km_unit::ratio::num, 1000);
}

TEST(UnitsQuantity, ConstructionAndLimits)
{
    constexpr meters<int> zero_init;
    static_assert(zero_init.value() == 0, "default zero");
    constexpr meters<double> m(2.5);
    static_assert(m.value() == 2.5, "value");
    EXPECT_EQ(meters<int>::zero().value(), 0);
    EXPECT_EQ(meters<int>::min().value(), castle::numeric_limits<int>::lowest());
    EXPECT_EQ(meters<int>::max().value(), castle::numeric_limits<int>::max());
    EXPECT_EQ(meters<double>::min().value(), castle::numeric_limits<double>::lowest());
    meters<int> copy(zero_init);
    copy = meters<int>(7);
    EXPECT_EQ(copy.value(), 7);
}

TEST(UnitsQuantity, ImplicitAndExplicitConversions)
{
    static_assert(is_lossless_conversion<km_unit, int, length_unit, int>::value, "km->m exact");
    static_assert(!is_lossless_conversion<length_unit, int, km_unit, int>::value, "m->km truncates");
    static_assert(is_lossless_conversion<length_unit, int, km_unit, double>::value, "floating target");
    static_assert(!is_lossless_conversion<length_unit, double, length_unit, int>::value, "float->int");
    static_assert(!is_lossless_conversion<deg_unit, int, rad_unit, int>::value, "pi factor");
    static_assert(!is_lossless_conversion<rad_unit, double, kinded_rad_unit, double>::value, "kind");
    static_assert(!is_lossless_conversion<rad_unit, double, length_unit, double>::value, "dimension");
    static_assert(is_lossless_conversion<celsius_unit, int, kelvin_unit, double>::value, "offset float");
    static_assert(!is_lossless_conversion<celsius_unit, int, kelvin_unit, int>::value, "offset fraction");
    static_assert(!castle::meta::is_convertible<meters<double>, radians<double>>::value, "no mixing");
    static_assert(!castle::meta::is_constructible<radians<double>, meters<double>>::value, "no mixing");
    static_assert(!castle::meta::is_constructible<kinded_radians<double>, radians<double>>::value, "kind");
    static_assert(castle::meta::is_convertible<kilometers<int>, meters<int>>::value, "implicit");
    static_assert(!castle::meta::is_convertible<meters<int>, kilometers<int>>::value, "explicit only");
    static_assert(castle::meta::is_constructible<kilometers<int>, meters<int>>::value, "explicit");
    static_assert(!castle::meta::is_convertible<double, meters<double>>::value, "raw explicit");

    constexpr meters<int> m = kilometers<int>(3);
    static_assert(m.value() == 3000, "km->m");
    EXPECT_EQ(m.value(), 3000);
    EXPECT_EQ(kilometers<int>(meters<int>(2999)).value(), 2);
    EXPECT_EQ(kilometers<int>(meters<int>(-2999)).value(), -2);
    EXPECT_EQ(meters<int>(meters<double>(2.9)).value(), 2);

    const feet<double> f = meters<double>(0.3048);
    EXPECT_DOUBLE_EQ(f.value(), 1.0);

    const radians<double> r = degrees<double>(180.0);
    EXPECT_DOUBLE_EQ(r.value(), cm::pi<double>());
    const degrees<double> d = radians<double>(cm::pi<double>() / 2.0);
    EXPECT_DOUBLE_EQ(d.value(), 90.0);
    EXPECT_EQ(degrees<int>(radians<double>(1.0)).value(), 57);
    EXPECT_EQ(radians<float>(degrees<int>(180)).value(), cm::pi<float>());

    const kelvin<double> k = celsius<double>(20.0);
    EXPECT_DOUBLE_EQ(k.value(), 293.15);
    EXPECT_EQ(kelvin<int>(celsius<int>(20)).value(), 293);
    EXPECT_EQ(celsius<int>(kelvin<int>(300)).value(), 26);
    const fahrenheit<double> fh = celsius<double>(100.0);
    EXPECT_NEAR(fh.value(), 212.0, 1e-9);
    EXPECT_EQ(fahrenheit<int>(celsius<int>(100)).value(), 212);
    EXPECT_EQ(celsius<int>(fahrenheit<int>(212)).value(), 100);
}

TEST(UnitsQuantity, QuantityCastAcrossKindsAndDurations)
{
    constexpr kilometers<int> km = cu::quantity_cast<kilometers<int>>(meters<int>(1500));
    static_assert(km.value() == 1, "cast truncates");
    EXPECT_EQ(km.value(), 1);
    const kinded_radians<double> kr = cu::quantity_cast<kinded_radians<double>>(degrees<double>(90.0));
    EXPECT_DOUBLE_EQ(kr.value(), cm::pi<double>() / 2.0);
    const castle::chrono::milliseconds ms =
        cu::quantity_cast<castle::chrono::milliseconds>(castle::chrono::duration<double>(1.25));
    EXPECT_EQ(ms.count(), 1250);
}

TEST(UnitsQuantity, CompoundAssignment)
{
    meters<int> m(10);
    m += kilometers<int>(1);
    EXPECT_EQ(m.value(), 1010);
    m -= meters<int>(10);
    EXPECT_EQ(m.value(), 1000);
    m *= 3;
    EXPECT_EQ(m.value(), 3000);
    m /= 4;
    EXPECT_EQ(m.value(), 750);
}

TEST(UnitsQuantity, UnaryAdditiveAndCommonType)
{
    EXPECT_EQ((+meters<int>(4)).value(), 4);
    EXPECT_EQ((-meters<int>(4)).value(), -4);

    const auto sum = meters<int>(500) + kilometers<int>(1);
    static_assert(is_same<decltype(sum), const meters<int>>::value, "common unit");
    EXPECT_EQ(sum.value(), 1500);
    const auto diff = kilometers<int>(2) - meters<int>(1);
    EXPECT_EQ(diff.value(), 1999);

    // Feet and meters share the gcd/lcm common scale (1/1250 m): 1 ft = 381, 1 m = 1250.
    const auto mixed = feet<int>(1) + meters<int>(1);
    EXPECT_EQ(mixed.value(), 381 + 1250);

    // Different pi factors fall back to radians with a floating representation.
    const auto angle = degrees<int>(180) + radians<int>(1);
    static_assert(is_same<decltype(angle), const radians<double>>::value, "fallback");
    EXPECT_DOUBLE_EQ(angle.value(), cm::pi<double>() + 1.0);
    const auto angle_diff = degrees<float>(180.0F) - radians<float>(1.0F);
    static_assert(is_same<decltype(angle_diff), const radians<float>>::value, "fallback float");
    EXPECT_FLOAT_EQ(angle_diff.value(), cm::pi<float>() - 1.0F);

    // Same offset unit adds directly.
    EXPECT_EQ((celsius<int>(20) + celsius<int>(5)).value(), 25);
    EXPECT_EQ((celsius<int>(20) - celsius<int>(5)).value(), 15);

    using common = castle::meta::common_type_t<meters<int>, kilometers<double>>;
    static_assert(is_same<common, meters<double>>::value, "meta::common_type");
    EXPECT_TRUE((is_same<common, meters<double>>::value));
}

TEST(UnitsQuantity, Comparisons)
{
    EXPECT_TRUE(meters<int>(1000) == kilometers<int>(1));
    EXPECT_FALSE(meters<int>(999) == kilometers<int>(1));
    EXPECT_TRUE(meters<int>(999) != kilometers<int>(1));
    EXPECT_FALSE(meters<int>(1000) != kilometers<int>(1));
    EXPECT_TRUE(meters<int>(999) < kilometers<int>(1));
    EXPECT_FALSE(meters<int>(1000) < kilometers<int>(1));
    EXPECT_TRUE(meters<int>(1000) <= kilometers<int>(1));
    EXPECT_FALSE(meters<int>(1001) <= kilometers<int>(1));
    EXPECT_TRUE(meters<int>(1001) > kilometers<int>(1));
    EXPECT_FALSE(meters<int>(1000) > kilometers<int>(1));
    EXPECT_TRUE(meters<int>(1000) >= kilometers<int>(1));
    EXPECT_FALSE(meters<int>(999) >= kilometers<int>(1));
    // Absolute temperatures compare across offset units.
    EXPECT_TRUE(celsius<int>(0) < fahrenheit<int>(33));
    EXPECT_TRUE(celsius<int>(100) == fahrenheit<int>(212));
    EXPECT_NEAR(kelvin<double>(celsius<double>(-273.15)).value(), 0.0, 1e-9);
}

TEST(UnitsQuantity, ScalarArithmetic)
{
    const auto scaled = meters<int>(3) * 2.5;
    static_assert(is_same<decltype(scaled), const meters<double>>::value, "common rep");
    EXPECT_DOUBLE_EQ(scaled.value(), 7.5);
    EXPECT_EQ((2 * meters<int>(3)).value(), 6);
    EXPECT_EQ((meters<int>(9) / 2).value(), 4);

    const auto period = 1.0 / hertz<double>(4.0);
    static_assert(is_same<decltype(period), const castle::chrono::duration<double>>::value, "1/Hz");
    EXPECT_DOUBLE_EQ(period.count(), 0.25);
    const auto per_meter = 2.0 / meters<double>(4.0);
    EXPECT_DOUBLE_EQ(per_meter.value(), 0.5);
    static_assert(decltype(per_meter)::dimension::length == -1, "inverse length");
}

TEST(UnitsQuantity, DerivedProductsAndQuotients)
{
    const mps<double> v = kilometers<double>(1.5) / castle::chrono::duration<double>(60.0);
    EXPECT_DOUBLE_EQ(v.value(), 25.0);

    const auto distance = v * castle::chrono::duration<double>(4.0);
    static_assert(is_same<decltype(distance), const meters<double>>::value, "v*t");
    EXPECT_DOUBLE_EQ(distance.value(), 100.0);
    const auto distance2 = castle::chrono::duration<double>(4.0) * v;
    EXPECT_DOUBLE_EQ(distance2.value(), 100.0);

    const auto time = meters<double>(100.0) / v;
    static_assert(is_same<decltype(time), const castle::chrono::duration<double>>::value, "d/v");
    EXPECT_DOUBLE_EQ(time.count(), 4.0);

    const auto rate = castle::chrono::duration<double>(2.0) / meters<double>(4.0);
    EXPECT_DOUBLE_EQ(rate.value(), 0.5);

    // Dimensionless results collapse to the coherent scale.
    const double ratio = meters<double>(500.0) / kilometers<double>(1.0);
    EXPECT_DOUBLE_EQ(ratio, 0.5);
    const double cycles = hertz<double>(50.0) * castle::chrono::milliseconds(100);
    EXPECT_DOUBLE_EQ(cycles, 5.0);
    const double turns = degrees<double>(90.0) / radians<double>(cm::pi<double>());
    EXPECT_DOUBLE_EQ(turns, 0.5);

    const auto area = meters<int>(3) * millimeters<int>(500);
    static_assert(decltype(area)::dimension::length == 2, "area");
    EXPECT_EQ(area.value(), 1500);
    using area_unit = typename decltype(area)::unit_type;
    static_assert(area_unit::ratio::den == 1000, "area scale");
}

TEST(UnitsQuantity, InverseAndAbs)
{
    const hertz<double> f = cu::inverse(castle::chrono::duration<double>(0.02));
    EXPECT_DOUBLE_EQ(f.value(), 50.0);
    const auto t = cu::inverse(hertz<double>(50.0));
    EXPECT_DOUBLE_EQ(t.count(), 0.02);
    const auto per_km = cu::inverse(kilometers<double>(2.0));
    EXPECT_DOUBLE_EQ(per_km.value(), 0.5);

    EXPECT_EQ(cu::abs(meters<int>(-5)).value(), 5);
    EXPECT_EQ(cu::abs(meters<int>(5)).value(), 5);
    EXPECT_DOUBLE_EQ(cu::abs(meters<double>(-0.5)).value(), 0.5);
}

TEST(UnitsDetail, PiPowerBothDirections)
{
    EXPECT_DOUBLE_EQ(cu::detail::pi_power<double>(0), 1.0);
    EXPECT_DOUBLE_EQ(cu::detail::pi_power<double>(2), cm::pi<double>() * cm::pi<double>());
    EXPECT_DOUBLE_EQ(cu::detail::pi_power<double>(-1), 1.0 / cm::pi<double>());
    static_assert(cu::detail::pi_power<double>(1) == cm::pi<double>(), "constexpr");
}

} // namespace
