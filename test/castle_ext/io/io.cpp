#include <gtest/gtest.h>

#include "castle_ext/io/io.hpp"

TEST(IoUmbrella, ExposesAllTransportAvailabilityFlags)
{
    EXPECT_EQ(castle::io::fd::descriptor_available, castle::io::fd::posix_available);
    EXPECT_EQ(castle::io::uart::uart_available, (CASTLE_USING_POSIX_APIS != 0));
#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    EXPECT_EQ(castle::io::spi::validate_config(castle::io::spi::spi_config("/dev/spidev0.0")),
              castle::status::ok);
    EXPECT_EQ(castle::io::i2c::validate_config(castle::io::i2c::i2c_config("/dev/i2c-1", 0x42U)),
              castle::status::ok);
#else
    EXPECT_FALSE(castle::io::spi::spi_available);
    EXPECT_FALSE(castle::io::i2c::i2c_available);
#endif
    SUCCEED();
}
