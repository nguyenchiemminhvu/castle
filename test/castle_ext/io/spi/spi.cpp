#include <gtest/gtest.h>

#include "castle_ext/io/spi/spi.hpp"

namespace
{

using namespace castle::io::spi;

TEST(Spi, ValidatesConfiguration)
{
    EXPECT_EQ(validate_config(spi_config()), castle::status::invalid_config);
    EXPECT_EQ(validate_config(spi_config("/dev/spidev0.0")), castle::status::ok);
    EXPECT_EQ(validate_config(spi_config("/dev/spidev0.0", static_cast<mode>(4U))), castle::status::invalid_config);
    EXPECT_EQ(validate_config(spi_config("/dev/spidev0.0", mode::mode0, 0U)), castle::status::invalid_config);
    EXPECT_EQ(validate_config(spi_config("/dev/spidev0.0", mode::mode0, 33U)), castle::status::invalid_config);
    EXPECT_EQ(validate_config(spi_config("/dev/spidev0.0", mode::mode0, 8U, 0U)), castle::status::invalid_config);
}

TEST(Spi, ClosedDeviceAndZeroLengthTransfer)
{
    basic_spi bus;
    EXPECT_FALSE(bus.is_open());
    EXPECT_FALSE(bus.is_configured());
    EXPECT_EQ(bus.configure(spi_config("/dev/spidev0.0")), castle::status::not_configured);
    EXPECT_EQ(bus.transfer(nullptr, nullptr, 0U), castle::status::ok);
    char byte = 0;
    EXPECT_EQ(bus.transfer(nullptr, &byte, 1U), castle::status::not_configured);
    EXPECT_EQ(bus.transfer(nullptr, nullptr, 1U), castle::status::invalid_argument);
    castle::size_type count = 0U;
    EXPECT_EQ(bus.read_some(&byte, 1U, count), castle::status::not_configured);
    EXPECT_EQ(count, 0U);
}

#if CASTLE_USING_POSIX_APIS && defined(__linux__)

TEST(Spi, LinuxSpidevFailureAndSystemError)
{
    basic_spi bus;
    int system_error = 0;
    EXPECT_EQ(bus.open(spi_config("/castle/no-such-spidev"), &system_error), castle::status::system_call_error);
    EXPECT_NE(system_error, 0);
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.open(spi_config("/dev/null"), &system_error), castle::status::system_call_error);
    EXPECT_NE(system_error, 0);
    EXPECT_FALSE(bus.is_open());
}

#else

TEST(Spi, LinuxSupportUnavailable)
{
    basic_spi bus;
    EXPECT_FALSE(spi_available);
    EXPECT_EQ(bus.open(spi_config("/dev/spidev0.0")), castle::status::not_configured);
}

#endif

} // namespace
