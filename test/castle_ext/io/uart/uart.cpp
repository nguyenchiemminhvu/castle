#define _XOPEN_SOURCE 600
#include <gtest/gtest.h>

#include "castle_ext/io/uart/uart.hpp"

#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    #include <fcntl.h>
    #include <stdlib.h>
    #include <unistd.h>
#endif

namespace
{

using namespace castle::io::uart;

TEST(Uart, ValidatesConfigurationFields)
{
    EXPECT_EQ(validate_config(uart_config()), castle::status::invalid_config);
    EXPECT_EQ(validate_config(uart_config("/dev/ttyS0")), castle::status::ok);
    EXPECT_EQ(validate_config(uart_config("/dev/ttyS0", 0U)), castle::status::invalid_config);
    EXPECT_EQ(validate_config(uart_config("/dev/ttyS0", 12345U)), castle::status::invalid_config);
    EXPECT_EQ(validate_config(uart_config("/dev/ttyS0", 115200U, static_cast<data_bits>(9U))),
              castle::status::invalid_config);
    EXPECT_EQ(validate_config(uart_config("/dev/ttyS0", 115200U, data_bits::eight,
                                          static_cast<parity>(99U))), castle::status::invalid_config);
    EXPECT_EQ(validate_config(uart_config("/dev/ttyS0", 115200U, data_bits::eight,
                                          parity::none, static_cast<stop_bits>(0U))),
              castle::status::invalid_config);
    EXPECT_EQ(validate_config(uart_config("/dev/ttyS0", 115200U, data_bits::eight,
                                          parity::none, stop_bits::one,
                                          static_cast<flow_control>(99U))),
              castle::status::invalid_config);
}

TEST(Uart, ClosedDeviceReportsNotConfigured)
{
    basic_uart serial;
    EXPECT_FALSE(serial.is_open());
    EXPECT_FALSE(serial.is_configured());
    castle::size_type count = 0U;
    char byte = 0;
    EXPECT_EQ(serial.read_some(&byte, 1U, count), castle::status::not_configured);
    EXPECT_EQ(count, 0U);
    EXPECT_EQ(serial.write_some(&byte, 1U, count), castle::status::not_configured);
    EXPECT_EQ(serial.flush_input(), castle::status::not_configured);
    EXPECT_EQ(serial.flush_output(), castle::status::not_configured);
    EXPECT_EQ(serial.flush(), castle::status::not_configured);
    EXPECT_EQ(serial.drain(), castle::status::not_configured);
    EXPECT_EQ(serial.bytes_available(count), castle::status::not_configured);
    EXPECT_EQ(serial.close(), castle::status::not_configured);
    EXPECT_EQ(serial.configure(uart_config("/dev/ttyS0")), castle::status::not_configured);
}

#if CASTLE_USING_POSIX_APIS && defined(__linux__)

TEST(Uart, OpensPseudoTerminalAndPerformsByteIo)
{
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    ASSERT_GE(master, 0);
    ASSERT_EQ(::grantpt(master), 0);
    ASSERT_EQ(::unlockpt(master), 0);
    char* slave_path = ::ptsname(master);
    ASSERT_NE(slave_path, nullptr);

    const uart_config config(slave_path, 115200U, data_bits::eight,
                             parity::none, stop_bits::one, flow_control::none,
                             0U, 0U, true);
    basic_uart serial;
    ASSERT_EQ(serial.open(config), castle::status::ok);
    EXPECT_TRUE(serial.is_open());
    EXPECT_TRUE(serial.is_configured());
    EXPECT_GE(serial.native_handle(), 0);
    EXPECT_EQ(serial.configuration().baud_rate, 115200U);
    EXPECT_EQ(serial.configure(config), castle::status::ok);
    EXPECT_EQ(serial.flush_input(), castle::status::ok);
    EXPECT_EQ(serial.flush_output(), castle::status::ok);
    EXPECT_EQ(serial.flush(), castle::status::ok);

    const char tx[] = {'T', 'E', 'S', 'T'};
    ASSERT_EQ(::write(master, tx, sizeof(tx)), static_cast<ssize_t>(sizeof(tx)));
    char rx[sizeof(tx)] = {};
    castle::size_type count = 0U;
    ASSERT_EQ(serial.read_some(rx, sizeof(rx), count), castle::status::ok);
    EXPECT_EQ(count, sizeof(rx));
    EXPECT_EQ(rx[0], 'T');
    EXPECT_EQ(rx[3], 'T');

    EXPECT_EQ(serial.bytes_available(count), castle::status::ok);
    EXPECT_EQ(serial.write_all(tx, sizeof(tx), count), castle::status::ok);
    EXPECT_EQ(count, sizeof(tx));
    EXPECT_EQ(serial.drain(), castle::status::ok);
    basic_uart moved(castle::move(serial));
    EXPECT_FALSE(serial.is_open());
    EXPECT_FALSE(serial.is_configured());
    EXPECT_TRUE(moved.is_open());
    EXPECT_TRUE(moved.is_configured());

    basic_uart assigned;
    assigned = castle::move(moved);
    EXPECT_FALSE(moved.is_open());
    EXPECT_FALSE(moved.is_configured());
    EXPECT_TRUE(assigned.is_open());
    EXPECT_TRUE(assigned.is_configured());
    EXPECT_EQ(assigned.close(), castle::status::ok);
    EXPECT_FALSE(assigned.is_open());
    EXPECT_FALSE(assigned.is_configured());
    EXPECT_EQ(::close(master), 0);
}

TEST(Uart, ReconfiguresSupportedBaudsAndLineSettings)
{
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    ASSERT_GE(master, 0);
    ASSERT_EQ(::grantpt(master), 0);
    ASSERT_EQ(::unlockpt(master), 0);
    char* slave_path = ::ptsname(master);
    ASSERT_NE(slave_path, nullptr);

    basic_uart serial;
    uart_config config(slave_path);
    ASSERT_EQ(serial.open(config), castle::status::ok);

    const uint32_t bauds[] = {50U, 75U, 110U, 134U, 150U, 200U, 300U, 600U,
        1200U, 1800U, 2400U, 4800U, 9600U, 19200U, 38400U, 57600U,
        115200U, 230400U, 460800U, 500000U, 576000U, 921600U, 1000000U,
        1152000U, 1500000U, 2000000U, 2500000U, 3000000U, 3500000U, 4000000U};
    for (castle::size_type i = 0U; i < sizeof(bauds) / sizeof(bauds[0]); ++i)
    {
        config.baud_rate = bauds[i];
        if (validate_config(config) == castle::status::ok)
        {
            EXPECT_EQ(serial.configure(config), castle::status::ok) << bauds[i];
        }
    }

    const data_bits bits[] = {data_bits::five, data_bits::six, data_bits::seven, data_bits::eight};
    for (castle::size_type i = 0U; i < sizeof(bits) / sizeof(bits[0]); ++i)
    {
        config.data = bits[i];
        const castle::status result = serial.configure(config);
        EXPECT_TRUE(result == castle::status::ok || result == castle::status::system_call_error);
    }
    config.parity_mode = parity::even;
    castle::status setting_result = serial.configure(config);
    EXPECT_TRUE(setting_result == castle::status::ok || setting_result == castle::status::system_call_error);
    config.parity_mode = parity::odd;
    setting_result = serial.configure(config);
    EXPECT_TRUE(setting_result == castle::status::ok || setting_result == castle::status::system_call_error);
    config.parity_mode = parity::mark;
    if (validate_config(config) == castle::status::ok)
    {
        setting_result = serial.configure(config);
        EXPECT_TRUE(setting_result == castle::status::ok || setting_result == castle::status::system_call_error);
    }
    config.parity_mode = parity::space;
    if (validate_config(config) == castle::status::ok)
    {
        setting_result = serial.configure(config);
        EXPECT_TRUE(setting_result == castle::status::ok || setting_result == castle::status::system_call_error);
    }
    config.parity_mode = parity::none;

    config.stop = stop_bits::two;
    setting_result = serial.configure(config);
    EXPECT_TRUE(setting_result == castle::status::ok || setting_result == castle::status::system_call_error);
    config.stop = stop_bits::one;
    config.flow = flow_control::software;
    setting_result = serial.configure(config);
    EXPECT_TRUE(setting_result == castle::status::ok || setting_result == castle::status::system_call_error);
    config.flow = flow_control::hardware;
    if (validate_config(config) == castle::status::ok)
    {
        setting_result = serial.configure(config);
        EXPECT_TRUE(setting_result == castle::status::ok || setting_result == castle::status::system_call_error);
    }
    EXPECT_EQ(serial.close(), castle::status::ok);
    EXPECT_EQ(::close(master), 0);
}

TEST(Uart, FailedOpenAndNonBlockingRead)
{
    basic_uart serial;
    int system_error = 0;
    EXPECT_EQ(serial.open(uart_config("/castle/no-such-tty"), &system_error), castle::status::system_call_error);
    EXPECT_NE(system_error, 0);
    EXPECT_FALSE(serial.is_open());
    EXPECT_EQ(serial.open(uart_config("/dev/null")), castle::status::system_call_error);
    EXPECT_FALSE(serial.is_open());
}

#else

TEST(Uart, PosixDisabledReturnsNotConfigured)
{
    basic_uart serial;
    EXPECT_EQ(serial.open(uart_config("/dev/ttyS0")), castle::status::not_configured);
}

#endif

} // namespace
