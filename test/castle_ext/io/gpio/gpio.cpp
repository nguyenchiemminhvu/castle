#include <gtest/gtest.h>

#include "castle_ext/io/gpio/gpio.hpp"
#include "castle_ext/io/gpio/backends/mock.hpp"

TEST(GpioUmbrella, ExposesTypesPinAndPortFacades)
{
    using namespace castle::io::gpio;
    mock::pin_state pin_memory;
    pin<mock::backend> output(&pin_memory);
    EXPECT_EQ(output.configure(pin_config(direction::output)), castle::status::ok);
    EXPECT_EQ(output.write(level::high), castle::status::ok);
    bool high = false;
    EXPECT_EQ(output.read(high), castle::status::ok);
    EXPECT_TRUE(high);

    mock::port_state port_memory;
    port<mock::port_backend> port_output(&port_memory);
    EXPECT_EQ(port_output.set(0x05U), castle::status::ok);
    mask_type value = 0U;
    EXPECT_EQ(port_output.read(value), castle::status::ok);
    EXPECT_EQ(value, 0x05U);
}
