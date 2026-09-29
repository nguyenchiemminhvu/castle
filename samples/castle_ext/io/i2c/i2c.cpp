#include <assert.h>

#include "castle_ext/io/i2c/i2c.hpp"

int main()
{
    using namespace castle::io::i2c;
    const i2c_config config("/dev/i2c-1", 0x42U);
    assert(validate_config(config) == castle::status::ok);

    basic_i2c bus;
    assert(!bus.is_open() && !bus.is_configured());
    uint32_t capabilities = 0U;
    assert(bus.get_functionality(capabilities) == castle::status::not_configured);
    assert(capabilities == 0U);
    assert(bus.configure(config) == castle::status::not_configured);
    assert(bus.close() == castle::status::not_configured);
    assert(bus.write_read(nullptr, 0U, nullptr, 0U) == castle::status::invalid_argument);
    char tx = 0x10;
    char rx = 0;
    castle::size_type count = 0U;
    assert(bus.read_some(&rx, 1U, count) == castle::status::not_configured);
    assert(count == 0U);
    assert(bus.write_some(&tx, 1U, count) == castle::status::not_configured);
    assert(count == 0U);
    assert(bus.write_read(&tx, 1U, &rx, 1U) == castle::status::not_configured);

#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    // /dev/null opens but rejects I2C address-selection ioctls; no bus transaction occurs.
    const i2c_config invalid_device("/dev/null", 0x42U);
    const castle::status opened = bus.open(invalid_device);
    assert(opened == castle::status::system_call_error || opened == castle::status::not_configured);
    assert(!bus.is_open());
#else
    assert(!i2c_available);
    assert(bus.open(config) == castle::status::not_configured);
#endif
    return 0;
}
