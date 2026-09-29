#include <gtest/gtest.h>

#include "castle_ext/gpio/pin.hpp"
#include "castle_ext/gpio/backends/mock.hpp"

namespace
{

using namespace castle::gpio;

TEST(GpioPin, HasNoHiddenStorage)
{
    static_assert(sizeof(basic_pin<mock::backend>) == sizeof(mock::backend::native_handle_type),
                  "basic_pin must not add hidden storage");
    SUCCEED();
}

TEST(GpioPin, ConfigureRecordsRequestedSettings)
{
    mock::pin_state state;
    basic_pin<mock::backend> p(&state);

    const pin_config cfg(direction::output, level::high, pull::down, drive::open_source, true);
    ASSERT_EQ(p.configure(cfg), castle::status::ok);

    EXPECT_TRUE(state.configured);
    EXPECT_TRUE(state.config.is_output());
    EXPECT_EQ(state.config.pull_mode, pull::down);
    EXPECT_EQ(state.config.drive_mode, drive::open_source);
}

TEST(GpioPin, WriteThenReadRoundTrips)
{
    mock::pin_state state;
    basic_pin<mock::backend> p(&state);

    ASSERT_EQ(p.write(level::high), castle::status::ok);
    level observed = level::low;
    ASSERT_EQ(p.read(observed), castle::status::ok);
    EXPECT_EQ(observed, level::high);

    ASSERT_EQ(p.write(level::low), castle::status::ok);
    ASSERT_EQ(p.read(observed), castle::status::ok);
    EXPECT_EQ(observed, level::low);
}

TEST(GpioPin, BoolOverloadsRoundTrip)
{
    mock::pin_state state;
    basic_pin<mock::backend> p(&state);

    ASSERT_EQ(p.write(true), castle::status::ok);
    bool observed = false;
    ASSERT_EQ(p.read(observed), castle::status::ok);
    EXPECT_TRUE(observed);

    ASSERT_EQ(p.write(false), castle::status::ok);
    ASSERT_EQ(p.read(observed), castle::status::ok);
    EXPECT_FALSE(observed);
}

TEST(GpioPin, Toggle)
{
    mock::pin_state state(/* active_low */ false, level::low);
    basic_pin<mock::backend> p(&state);

    ASSERT_EQ(p.toggle(), castle::status::ok);
    level observed = level::low;
    ASSERT_EQ(p.read(observed), castle::status::ok);
    EXPECT_EQ(observed, level::high);

    ASSERT_EQ(p.toggle(), castle::status::ok);
    ASSERT_EQ(p.read(observed), castle::status::ok);
    EXPECT_EQ(observed, level::low);
}

TEST(GpioPin, ActiveLowInvertsLogicalButNotRaw)
{
    mock::pin_state state(/* active_low */ true, level::high);
    basic_pin<mock::backend> p(&state);

    level raw = level::low;
    ASSERT_EQ(p.read_raw(raw), castle::status::ok);
    EXPECT_EQ(raw, level::high);

    level logical = level::low;
    ASSERT_EQ(p.read(logical), castle::status::ok);
    EXPECT_EQ(logical, level::low);

    ASSERT_EQ(p.write(level::high), castle::status::ok);
    ASSERT_EQ(p.read_raw(raw), castle::status::ok);
    EXPECT_EQ(raw, level::low);
}

TEST(GpioPin, ActiveLowToggleFlipsLogicalLevel)
{
    mock::pin_state state(/* active_low */ true, level::low);
    basic_pin<mock::backend> p(&state);

    level logical = level::low;
    ASSERT_EQ(p.read(logical), castle::status::ok);
    EXPECT_EQ(logical, level::high);

    ASSERT_EQ(p.toggle(), castle::status::ok);
    ASSERT_EQ(p.read(logical), castle::status::ok);
    EXPECT_EQ(logical, level::low);
}

TEST(GpioPin, WriteRawBypassesActiveLow)
{
    mock::pin_state state(/* active_low */ true, level::low);
    basic_pin<mock::backend> p(&state);

    ASSERT_EQ(p.write_raw(level::high), castle::status::ok);
    level raw = level::low;
    ASSERT_EQ(p.read_raw(raw), castle::status::ok);
    EXPECT_EQ(raw, level::high);

    level logical = level::low;
    ASSERT_EQ(p.read(logical), castle::status::ok);
    EXPECT_EQ(logical, level::low);
}

TEST(GpioPin, NullHandleReportsInvalidArgument)
{
    basic_pin<mock::backend> p; // default-constructed native_handle_type is nullptr

    EXPECT_EQ(p.write(level::high), castle::status::invalid_argument);

    level observed = level::low;
    EXPECT_EQ(p.read(observed), castle::status::invalid_argument);
    EXPECT_EQ(p.toggle(), castle::status::invalid_argument);
}

TEST(GpioPin, ResetRebindsHandle)
{
    mock::pin_state state_a(/* active_low */ false, level::low);
    mock::pin_state state_b(/* active_low */ false, level::high);

    basic_pin<mock::backend> p(&state_a);
    p.reset(&state_b);

    level observed = level::low;
    ASSERT_EQ(p.read(observed), castle::status::ok);
    EXPECT_EQ(observed, level::high);
}

} // namespace

