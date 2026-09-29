#include <gtest/gtest.h>

#include "castle_ext/io/i2c/i2c.hpp"

namespace
{

using namespace castle::io::i2c;

TEST(I2c, ValidatesSevenAndTenBitAddressRanges)
{
    EXPECT_EQ(validate_config(i2c_config()), castle::status::invalid_config);
    EXPECT_EQ(validate_config(i2c_config("/dev/i2c-1", 0x42U)), castle::status::ok);
    EXPECT_EQ(validate_config(i2c_config("/dev/i2c-1", 0x80U)), castle::status::invalid_config);
    EXPECT_EQ(validate_config(i2c_config("/dev/i2c-1", 0x3FFU, true)), castle::status::ok);
    EXPECT_EQ(validate_config(i2c_config("/dev/i2c-1", 0x400U, true)), castle::status::invalid_config);
}

TEST(I2c, ClosedDeviceRejectsOperations)
{
    basic_i2c bus;
    EXPECT_FALSE(bus.is_open());
    EXPECT_FALSE(bus.is_configured());
    uint32_t flags = 10U;
    EXPECT_EQ(bus.get_functionality(flags), castle::status::not_configured);
    EXPECT_EQ(flags, 0U);
    char buffer[2] = {};
    castle::size_type count = 0U;
    EXPECT_EQ(bus.read_some(buffer, sizeof(buffer), count), castle::status::not_configured);
    EXPECT_EQ(count, 0U);
    EXPECT_EQ(bus.write_some(buffer, sizeof(buffer), count), castle::status::not_configured);
    EXPECT_EQ(count, 0U);
    EXPECT_EQ(bus.write_read(buffer, 1U, buffer, 1U), castle::status::not_configured);
    EXPECT_EQ(bus.configure(i2c_config("/dev/i2c-1", 0x42U)), castle::status::not_configured);
    EXPECT_EQ(bus.close(), castle::status::not_configured);
}

TEST(I2c, RejectsInvalidCombinedTransfers)
{
    basic_i2c bus;
    char buffer = 0;
    EXPECT_EQ(bus.write_read(nullptr, 1U, &buffer, 1U), castle::status::invalid_argument);
    EXPECT_EQ(bus.write_read(&buffer, 0U, &buffer, 1U), castle::status::invalid_argument);
    EXPECT_EQ(bus.write_read(&buffer, 1U, nullptr, 1U), castle::status::invalid_argument);
    EXPECT_EQ(bus.write_read(&buffer, 1U, &buffer, 0U), castle::status::invalid_argument);
    EXPECT_EQ(bus.write_read(&buffer, 65536U, &buffer, 1U), castle::status::invalid_argument);
}

#if CASTLE_USING_POSIX_APIS && defined(__linux__)

TEST(I2c, LinuxI2cDevFailureAndSystemError)
{
    basic_i2c bus;
    int system_error = 0;
    EXPECT_EQ(bus.open(i2c_config("/castle/no-such-i2c", 0x42U), &system_error), castle::status::system_call_error);
    EXPECT_NE(system_error, 0);
    EXPECT_FALSE(bus.is_open());
    EXPECT_EQ(bus.open(i2c_config("/dev/null", 0x42U), &system_error), castle::status::system_call_error);
    EXPECT_NE(system_error, 0);
    EXPECT_FALSE(bus.is_open());
}

#else

TEST(I2c, LinuxSupportUnavailable)
{
    basic_i2c bus;
    EXPECT_FALSE(i2c_available);
    EXPECT_EQ(bus.open(i2c_config("/dev/i2c-1", 0x42U)), castle::status::not_configured);
}

#endif

} // namespace
