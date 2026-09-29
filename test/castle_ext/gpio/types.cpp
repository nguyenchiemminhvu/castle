#include <gtest/gtest.h>

#include "castle_ext/gpio/types.hpp"

namespace
{

using namespace castle::gpio;

TEST(GpioTypes, IsHighIsLow)
{
    EXPECT_TRUE(is_high(level::high));
    EXPECT_FALSE(is_high(level::low));
    EXPECT_TRUE(is_low(level::low));
    EXPECT_FALSE(is_low(level::high));
}

TEST(GpioTypes, ToLevelToBool)
{
    EXPECT_EQ(to_level(true), level::high);
    EXPECT_EQ(to_level(false), level::low);
    EXPECT_TRUE(to_bool(level::high));
    EXPECT_FALSE(to_bool(level::low));
}

TEST(GpioTypes, Invert)
{
    EXPECT_EQ(invert(level::high), level::low);
    EXPECT_EQ(invert(level::low), level::high);
    EXPECT_EQ(invert(invert(level::high)), level::high);
}

TEST(GpioTypes, PinConfigDefaults)
{
    constexpr pin_config cfg;

    static_assert(cfg.is_input(), "default direction is input");
    static_assert(!cfg.is_output(), "default direction is not output");

    EXPECT_TRUE(cfg.is_input());
    EXPECT_FALSE(cfg.is_output());
    EXPECT_EQ(cfg.pull_mode, pull::none);
    EXPECT_EQ(cfg.drive_mode, drive::push_pull);
    EXPECT_EQ(cfg.initial, level::low);
    EXPECT_FALSE(cfg.initial_valid);
}

TEST(GpioTypes, PinConfigOutput)
{
    constexpr pin_config cfg(direction::output, level::high, pull::up, drive::open_drain, true);

    static_assert(cfg.is_output(), "explicit output direction");
    static_assert(!cfg.is_input(), "explicit output direction");

    EXPECT_TRUE(cfg.is_output());
    EXPECT_EQ(cfg.pull_mode, pull::up);
    EXPECT_EQ(cfg.drive_mode, drive::open_drain);
    EXPECT_EQ(cfg.initial, level::high);
    EXPECT_TRUE(cfg.initial_valid);
}

} // namespace

