#include <gtest/gtest.h>

#include "castle/units/units.hpp"

#include <stdint.h>

namespace
{

namespace cu = castle::units;
using namespace castle::units::literals;
using castle::meta::is_same;

template <typename A, typename B, typename = void>
struct can_add : castle::meta::false_type
{
};

template <typename A, typename B>
struct can_add<A, B, castle::meta::void_t<decltype(castle::meta::declval<A>() + castle::meta::declval<B>())>>
    : castle::meta::true_type
{
};

cu::meters_per_second<double> echo_velocity(cu::meters_per_second<double> speed)
{
    return speed;
}

TEST(UnitsUmbrella, FunctionParametersAcceptOnlyMatchingQuantities)
{
    EXPECT_DOUBLE_EQ(echo_velocity(36.0_kmph).value(), 10.0);
    EXPECT_DOUBLE_EQ(echo_velocity(100.0_m / 4_s).value(), 25.0);
    static_assert(!castle::meta::is_convertible<cu::degrees<double>, cu::meters_per_second<double>>::value, "angle");
    static_assert(!castle::meta::is_convertible<double, cu::meters_per_second<double>>::value, "raw double");
    static_assert(!castle::meta::is_convertible<cu::hertz<double>, cu::radians_per_second<double>>::value,
                  "Hz is not rad/s");
    static_assert(!castle::meta::is_convertible<cu::degrees<double>, cu::geo_degrees<double>>::value,
                  "plane angle is not a geographic coordinate");
    static_assert(!can_add<cu::volts<double>, cu::amperes<double>>::value, "V + A");
    static_assert(can_add<cu::volts<double>, cu::millivolts<int>>::value, "V + mV");
    EXPECT_FALSE((can_add<cu::meters<double>, cu::seconds<double>>::value));
}

TEST(UnitsUmbrella, ElectricalIdentities)
{
    const cu::watts<double> p = 12.0_V * 2.0_A;
    EXPECT_DOUBLE_EQ(p.value(), 24.0);
    const cu::ohms<double> r = 5.0_V / 10.0_mA;
    EXPECT_DOUBLE_EQ(r.value(), 500.0);
    const cu::coulombs<double> q = 2.0_A * 3.0_s;
    EXPECT_DOUBLE_EQ(q.value(), 6.0);
    const cu::milliampere_hours<double> drained = 500.0_mA * 2.0_h;
    EXPECT_DOUBLE_EQ(drained.value(), 1000.0);
    const cu::watt_hours<double> e = 10.0_W * 30.0_min;
    EXPECT_DOUBLE_EQ(e.value(), 5.0);
    const cu::joules<double> work = 10.0_N * 2.0_m;
    EXPECT_DOUBLE_EQ(work.value(), 20.0);
}

TEST(UnitsUmbrella, KinematicIdentities)
{
    const cu::meters_per_second_squared<double> a = 10.0_mps / 2.0_s;
    EXPECT_DOUBLE_EQ(a.value(), 5.0);
    const cu::newtons<double> f = 2.0_kg * a;
    EXPECT_DOUBLE_EQ(f.value(), 10.0);
    const cu::degrees_per_second<double> rate = 90.0_deg / 0.5_s;
    EXPECT_DOUBLE_EQ(rate.value(), 180.0);
    const cu::revolutions_per_minute<double> rpm = rate;
    EXPECT_DOUBLE_EQ(rpm.value(), 30.0);
    const cu::hertz<double> f_tick = cu::inverse(1.0_ms);
    EXPECT_DOUBLE_EQ(f_tick.value(), 1000.0);
    const auto period = 1.0 / 50.0_Hz;
    static_assert(is_same<decltype(period), const cu::seconds<double>>::value, "period");
    EXPECT_DOUBLE_EQ(period.count(), 0.02);
    const cu::pascals<double> p = 10.0_N / (2.0_m * 1.0_m);
    EXPECT_DOUBLE_EQ(p.value(), 5.0);
}

TEST(UnitsUmbrella, LiteralSuffixesCoexist)
{
    // Every header's literals in one scope without ambiguity.
    EXPECT_EQ((5_min).count(), 5);
    EXPECT_EQ((5_m).value(), 5);
    EXPECT_EQ((5_mi).value(), 5);
    EXPECT_EQ((5_mm).value(), 5);
    EXPECT_EQ((5_ms).count(), 5);
    EXPECT_EQ((5_mA).value(), 5);
    EXPECT_EQ((5_mHz).value(), 5);
    EXPECT_EQ((5_MHz).value(), 5);
    EXPECT_EQ((5_degC).value(), 5);
    EXPECT_EQ((5_dB).value(), 5);
    EXPECT_EQ((5_C).value(), 5);
    EXPECT_EQ((5_g).value(), 5);
    EXPECT_EQ((5_gn).value(), 5);
}

} // namespace
