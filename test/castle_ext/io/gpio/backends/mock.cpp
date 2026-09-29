#include <gtest/gtest.h>

#include "castle_ext/io/gpio/backends/mock.hpp"

TEST(GpioMockBackend, PinLogicalAndRawOperations)
{
    using namespace castle::io::gpio;
    mock::pin_state state(true, level::low);
    const pin_config config(direction::output, level::low, pull::up, drive::open_drain, true);
    EXPECT_EQ(mock::backend::configure(&state, config), castle::status::ok);
    EXPECT_TRUE(state.configured);
    EXPECT_EQ(mock::backend::write(&state, level::high), castle::status::ok);
    EXPECT_EQ(state.physical, level::low);

    level value = level::low;
    EXPECT_EQ(mock::backend::read(&state, value), castle::status::ok);
    EXPECT_EQ(value, level::high);
    EXPECT_EQ(mock::backend::write_raw(&state, level::high), castle::status::ok);
    EXPECT_EQ(mock::backend::read_raw(&state, value), castle::status::ok);
    EXPECT_EQ(value, level::high);
    EXPECT_EQ(mock::backend::toggle(&state), castle::status::ok);
    EXPECT_EQ(state.physical, level::low);
}

TEST(GpioMockBackend, PortMaskedOperationsAndPolarity)
{
    using namespace castle::io::gpio;
    mock::port_state state(0xA0U, 0x03U);
    mask_type value = 0U;
    EXPECT_EQ(mock::port_backend::read_raw(&state, value), castle::status::ok);
    EXPECT_EQ(value, 0xA0U);
    EXPECT_EQ(mock::port_backend::read(&state, value), castle::status::ok);
    EXPECT_EQ(value, 0xA3U);
    EXPECT_EQ(mock::port_backend::write(&state, 0x55U), castle::status::ok);
    EXPECT_EQ(state.physical, 0x56U);
    EXPECT_EQ(mock::port_backend::write_masked(&state, 0xF0U, 0x30U), castle::status::ok);
    EXPECT_EQ(mock::port_backend::set(&state, 0x04U), castle::status::ok);
    EXPECT_EQ(mock::port_backend::clear(&state, 0x04U), castle::status::ok);
    EXPECT_EQ(mock::port_backend::toggle(&state, 0x08U), castle::status::ok);
}
