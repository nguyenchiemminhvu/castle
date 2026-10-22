#include <assert.h>

#include "castle_ext/comm/can/can.hpp"
#include "castle_ext/io/io.hpp"

int main()
{
    castle::io::fd::basic_fd descriptor;
    castle::io::file::basic_file file;
    castle::io::uart::uart_config serial("/dev/ttyS0", 115200U);
    castle::io::spi::spi_config spi("/dev/spidev0.0");
    castle::io::i2c::i2c_config i2c("/dev/i2c-1", 0x42U);
    castle::comm::can::frame can_frame(0x123U);

    assert(castle::io::uart::validate_config(serial) == castle::status::ok);
    assert(castle::io::spi::validate_config(spi) == castle::status::ok);
    assert(castle::io::i2c::validate_config(i2c) == castle::status::ok);
    assert(castle::comm::can::validate_frame(can_frame) == castle::status::ok);
    assert(!descriptor.is_open() && !file.is_open());
    return 0;
}
