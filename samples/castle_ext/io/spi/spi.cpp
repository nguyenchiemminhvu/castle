#include <assert.h>

#include "castle_ext/io/spi/spi.hpp"

int main()
{
    using namespace castle::io::spi;
    const spi_config config("/dev/spidev0.0", mode::mode0, 8U, 1000000U);
    assert(validate_config(config) == castle::status::ok);

    basic_spi bus;
    assert(!bus.is_open() && !bus.is_configured());
    assert(bus.configure(config) == castle::status::not_configured);
    assert(bus.close() == castle::status::not_configured);
    // Zero-length transfer is a successful no-op and never touches hardware.
    assert(bus.transfer(nullptr, nullptr, 0U) == castle::status::ok);
    assert(bus.transfer(nullptr, nullptr, 1U) == castle::status::invalid_argument);
    const unsigned char byte = 0x9FU;
    unsigned char received = 0U;
    assert(bus.transfer(&byte, &received, 1U) == castle::status::not_configured);
    castle::size_type count = 0U;
    assert(bus.read_some(&received, 1U, count) == castle::status::not_configured);
    assert(count == 0U);
    assert(bus.write_some(&byte, 1U, count) == castle::status::not_configured);
    assert(count == 0U);

#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    // /dev/null opens but rejects the spidev ioctl, exercising the failure path.
    const spi_config invalid_device("/dev/null", mode::mode0, 8U, 1000000U);
    const castle::status opened = bus.open(invalid_device);
    assert(opened == castle::status::system_call_error || opened == castle::status::not_configured);
    assert(!bus.is_open());
#else
    assert(!spi_available);
    assert(bus.open(config) == castle::status::not_configured);
#endif
    return 0;
}
