#include <gtest/gtest.h>

#include "castle_ext/gpio/port.hpp"
#include "castle_ext/gpio/backends/mock.hpp"

namespace
{

using namespace castle::gpio;

TEST(GpioPort, HasNoHiddenStorage)
{
    static_assert(sizeof(basic_port<mock::port_backend>) == sizeof(mock::port_backend::native_handle_type),
                  "basic_port must not add hidden storage");
    SUCCEED();
}

TEST(GpioPort, WriteThenReadRoundTrips)
{
    mock::port_state state;
    basic_port<mock::port_backend> port(&state);

    ASSERT_EQ(port.write(0x00FFU), castle::status::ok);
    mask_type value = 0U;
    ASSERT_EQ(port.read(value), castle::status::ok);
    EXPECT_EQ(value, 0x00FFU);
}

TEST(GpioPort, SetAndClearTouchOnlySelectedBits)
{
    mock::port_state state(0x000FU);
    basic_port<mock::port_backend> port(&state);

    ASSERT_EQ(port.set(0x00F0U), castle::status::ok);
    mask_type value = 0U;
    ASSERT_EQ(port.read(value), castle::status::ok);
    EXPECT_EQ(value, 0x00FFU);

    ASSERT_EQ(port.clear(0x000FU), castle::status::ok);
    ASSERT_EQ(port.read(value), castle::status::ok);
    EXPECT_EQ(value, 0x00F0U);
}

TEST(GpioPort, WriteMaskedReplacesOnlySelectedBits)
{
    mock::port_state state(0x00FFU);
    basic_port<mock::port_backend> port(&state);

    ASSERT_EQ(port.write_masked(0x000FU, 0x000AU), castle::status::ok);
    mask_type value = 0U;
    ASSERT_EQ(port.read(value), castle::status::ok);
    EXPECT_EQ(value, 0x00FAU);
}

TEST(GpioPort, Toggle)
{
    mock::port_state state(0x0001U);
    basic_port<mock::port_backend> port(&state);

    ASSERT_EQ(port.toggle(0x0003U), castle::status::ok);
    mask_type value = 0U;
    ASSERT_EQ(port.read(value), castle::status::ok);
    EXPECT_EQ(value, 0x0002U);
}

TEST(GpioPort, ActiveLowInvertsLogicalButNotRaw)
{
    mock::port_state state(/* initial */ 0U, /* invert_mask */ 0x000FU);
    basic_port<mock::port_backend> port(&state);

    ASSERT_EQ(port.write(0x000FU), castle::status::ok);

    mask_type raw = 0U;
    ASSERT_EQ(port.read_raw(raw), castle::status::ok);
    EXPECT_EQ(raw, 0x0000U);

    mask_type logical = 0U;
    ASSERT_EQ(port.read(logical), castle::status::ok);
    EXPECT_EQ(logical, 0x000FU);
}

TEST(GpioPort, NullHandleReportsInvalidArgument)
{
    basic_port<mock::port_backend> port; // default-constructed native_handle_type is nullptr

    EXPECT_EQ(port.write(0x1U), castle::status::invalid_argument);

    mask_type value = 0U;
    EXPECT_EQ(port.read(value), castle::status::invalid_argument);
    EXPECT_EQ(port.set(0x1U), castle::status::invalid_argument);
    EXPECT_EQ(port.clear(0x1U), castle::status::invalid_argument);
    EXPECT_EQ(port.toggle(0x1U), castle::status::invalid_argument);
}

TEST(GpioPort, ResetRebindsHandle)
{
    mock::port_state state_a(0x0000U);
    mock::port_state state_b(0x00FFU);

    basic_port<mock::port_backend> port(&state_a);
    port.reset(&state_b);

    mask_type value = 0U;
    ASSERT_EQ(port.read(value), castle::status::ok);
    EXPECT_EQ(value, 0x00FFU);
}

} // namespace

