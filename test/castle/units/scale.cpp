#include <gtest/gtest.h>

#include "castle/units/scale.hpp"

namespace
{

namespace cu = castle::units;

TEST(UnitsScale, SiPrefixes)
{
    static_assert(cu::atto::num == 1 && cu::atto::den == 1000000000000000000LL, "atto");
    static_assert(cu::femto::den == 1000000000000000LL, "femto");
    static_assert(cu::pico::den == 1000000000000LL, "pico");
    static_assert(cu::nano::den == 1000000000LL, "nano");
    static_assert(cu::micro::den == 1000000LL, "micro");
    static_assert(cu::milli::num == 1 && cu::milli::den == 1000LL, "milli");
    static_assert(cu::centi::den == 100LL, "centi");
    static_assert(cu::deci::den == 10LL, "deci");
    static_assert(cu::deca::num == 10LL && cu::deca::den == 1, "deca");
    static_assert(cu::hecto::num == 100LL, "hecto");
    static_assert(cu::kilo::num == 1000LL, "kilo");
    static_assert(cu::mega::num == 1000000LL, "mega");
    static_assert(cu::giga::num == 1000000000LL, "giga");
    EXPECT_EQ(cu::milli::den, 1000);
    EXPECT_EQ(cu::kilo::num, 1000);
    EXPECT_TRUE((castle::math::ratio_equal<castle::math::ratio_multiply_t<cu::kilo, cu::milli>,
                                           castle::math::ratio<1>>::value));
}

TEST(UnitsScale, TimeScales)
{
    static_assert(cu::second::num == 1 && cu::second::den == 1, "second");
    static_assert(cu::minute::num == 60, "minute");
    static_assert(cu::hour::num == 3600, "hour");
    static_assert(cu::day::num == 86400, "day");
    static_assert(cu::week::num == 604800, "week");
    EXPECT_EQ(cu::hour::num / cu::minute::num, 60);
    EXPECT_EQ(cu::week::num / cu::day::num, 7);
}

} // namespace
