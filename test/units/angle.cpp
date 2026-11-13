#include <gtest/gtest.h>

#include "castle/units/angle.hpp"
#include "castle/units/distance.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
namespace cm = castle::math;
using namespace castle::units::literals;
using castle::meta::is_same;

struct test_kind
{
};
using kinded_degree_unit = cu::unit<cu::dim::angle, cm::ratio<1, 180>, 1, cm::ratio<0>, test_kind>;
template <typename Rep> using kinded_degrees = cu::quantity<kinded_degree_unit, Rep>;
using centidegree_unit = cu::scaled_unit_t<cu::degree_unit, cu::centi>;
template <typename Rep> using centidegrees = cu::quantity<centidegree_unit, Rep>;

TEST(UnitsAngle, UnitDefinitions)
{
    static_assert(is_same<cu::radian_unit::dimension, cu::dim::angle>::value, "dimension");
    static_assert(cu::degree_unit::pi_exponent == 1, "degree pi");
    static_assert(cu::revolution_unit::ratio::num == 2, "revolution");
    static_assert(cu::milliradian_unit::ratio::den == 1000, "mrad");
    EXPECT_EQ(cu::arcminute_unit::ratio::den, 10800);
    EXPECT_EQ(cu::arcsecond_unit::ratio::den, 648000);
}

TEST(UnitsAngle, ConversionsBetweenAngleUnits)
{
    const cu::radians<double> r = cu::degrees<double>(180.0);
    EXPECT_DOUBLE_EQ(r.value(), cm::pi<double>());
    const cu::degrees<double> d = cu::revolutions<double>(0.25);
    EXPECT_DOUBLE_EQ(d.value(), 90.0);
    const cu::arcminutes<int> am = cu::degrees<int>(2);
    EXPECT_EQ(am.value(), 120);
    const cu::arcseconds<int> as = cu::arcminutes<int>(2);
    EXPECT_EQ(as.value(), 120);
    const cu::milliradians<double> mr = cu::radians<double>(0.5);
    EXPECT_DOUBLE_EQ(mr.value(), 500.0);
    EXPECT_EQ(cu::degrees<int>(cu::revolutions<int>(3)).value(), 1080);
}

TEST(UnitsAngle, Literals)
{
    static_assert(is_same<decltype(1.5_rad), cu::radians<double>>::value, "rad");
    static_assert(is_same<decltype(90_deg), cu::degrees<int64_t>>::value, "deg");
    EXPECT_DOUBLE_EQ((1.5_rad).value(), 1.5);
    EXPECT_EQ((2_rad).value(), 2);
    EXPECT_DOUBLE_EQ((2.5_mrad).value(), 2.5);
    EXPECT_EQ((25_mrad).value(), 25);
    EXPECT_DOUBLE_EQ((45.5_deg).value(), 45.5);
    EXPECT_EQ((90_deg).value(), 90);
    EXPECT_DOUBLE_EQ((1.5_arcmin).value(), 1.5);
    EXPECT_EQ((30_arcmin).value(), 30);
    EXPECT_DOUBLE_EQ((1.5_arcsec).value(), 1.5);
    EXPECT_EQ((30_arcsec).value(), 30);
    EXPECT_DOUBLE_EQ((0.25_rev).value(), 0.25);
    EXPECT_EQ((3_rev).value(), 3);
    EXPECT_TRUE(180_deg == 0.5_rev);
}

TEST(UnitsAngle, Trigonometry)
{
    EXPECT_NEAR(cu::sin(30.0_deg), 0.5, 1e-12);
    EXPECT_NEAR(cu::cos(60.0_deg), 0.5, 1e-12);
    EXPECT_NEAR(cu::tan(45.0_deg), 1.0, 1e-12);
    EXPECT_NEAR(cu::sin(cu::radians<float>(cm::pi<float>() / 2.0F)), 1.0F, 1e-6F);
    // Integral angles evaluate in double; ADL finds the units overloads.
    static_assert(is_same<decltype(sin(90_deg)), double>::value, "integral -> double");
    EXPECT_NEAR(sin(90_deg), 1.0, 1e-12);
    // Kinded angles are angles too.
    EXPECT_NEAR(cu::cos(kinded_degrees<double>(180.0)), -1.0, 1e-12);

    const cu::radians<double> a = cu::asin(0.5);
    EXPECT_NEAR(cu::degrees<double>(a).value(), 30.0, 1e-12);
    const cu::radians<double> c = cu::acos(0.5);
    EXPECT_NEAR(cu::degrees<double>(c).value(), 60.0, 1e-12);
    const cu::radians<float> t = cu::atan2(1.0F, -1.0F);
    EXPECT_NEAR(cu::degrees<float>(t).value(), 135.0F, 1e-4F);
    const auto bearing = cu::atan2(cu::meters<double>(100.0), cu::kilometers<double>(0.1));
    static_assert(is_same<decltype(bearing), const cu::radians<double>>::value, "quantity atan2");
    EXPECT_NEAR(cu::degrees<double>(bearing).value(), 45.0, 1e-12);
    const auto int_bearing = cu::atan2(cu::meters<int>(-1), cu::meters<int>(0));
    EXPECT_NEAR(int_bearing.value(), -cm::pi<double>() / 2.0, 1e-12);
}

TEST(UnitsAngle, WrapPositiveFloating)
{
    EXPECT_DOUBLE_EQ(cu::wrap_positive(370.0_deg).value(), 10.0);
    EXPECT_DOUBLE_EQ(cu::wrap_positive(-10.0_deg).value(), 350.0);
    EXPECT_DOUBLE_EQ(cu::wrap_positive(360.0_deg).value(), 0.0);
    EXPECT_DOUBLE_EQ(cu::wrap_positive(45.0_deg).value(), 45.0);
    // A tiny negative angle rounds to one full turn and is folded back to zero.
    EXPECT_DOUBLE_EQ(cu::wrap_positive(cu::degrees<double>(-1e-20)).value(), 0.0);
    EXPECT_NEAR(cu::wrap_positive(cu::radians<double>(-cm::pi<double>() / 2.0)).value(),
                1.5 * cm::pi<double>(), 1e-12);
    EXPECT_FLOAT_EQ(cu::wrap_positive(cu::degrees<float>(725.0F)).value(), 5.0F);
    EXPECT_DOUBLE_EQ(cu::wrap_positive(cu::revolutions<long double>(2.25L)).value(), 0.25L);
    // Kind is preserved.
    static_assert(is_same<decltype(cu::wrap_positive(kinded_degrees<double>(1.0))), kinded_degrees<double>>::value,
                  "kind preserved");
}

TEST(UnitsAngle, WrapSignedFloating)
{
    EXPECT_DOUBLE_EQ(cu::wrap_signed(190.0_deg).value(), -170.0);
    EXPECT_DOUBLE_EQ(cu::wrap_signed(180.0_deg).value(), -180.0);
    EXPECT_DOUBLE_EQ(cu::wrap_signed(-180.0_deg).value(), -180.0);
    EXPECT_DOUBLE_EQ(cu::wrap_signed(-190.0_deg).value(), 170.0);
    EXPECT_DOUBLE_EQ(cu::wrap_signed(10.0_deg).value(), 10.0);
    EXPECT_NEAR(cu::wrap_signed(cu::radians<double>(1.5 * cm::pi<double>())).value(),
                -0.5 * cm::pi<double>(), 1e-12);
}

TEST(UnitsAngle, WrapIntegral)
{
    EXPECT_EQ(cu::wrap_positive(cu::degrees<int16_t>(370)).value(), 10);
    EXPECT_EQ(cu::wrap_positive(cu::degrees<int16_t>(-10)).value(), 350);
    EXPECT_EQ(cu::wrap_positive(cu::degrees<uint16_t>(720)).value(), 0);
    EXPECT_EQ(cu::wrap_signed(cu::degrees<int16_t>(190)).value(), -170);
    EXPECT_EQ(cu::wrap_signed(cu::degrees<int16_t>(180)).value(), -180);
    EXPECT_EQ(cu::wrap_signed(cu::degrees<int16_t>(-181)).value(), 179);
    EXPECT_EQ(cu::wrap_positive(centidegrees<int32_t>(-100)).value(), 35900);
    EXPECT_EQ(cu::wrap_signed(centidegrees<int32_t>(18000)).value(), -18000);
    EXPECT_EQ(cu::wrap_positive(cu::arcminutes<int32_t>(21601)).value(), 1);
}

} // namespace
