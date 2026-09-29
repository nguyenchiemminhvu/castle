#define _XOPEN_SOURCE 600
#include <assert.h>

#include "castle_ext/io/uart/uart.hpp"

#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    #include <fcntl.h>
    #include <stdlib.h>
    #include <unistd.h>
#endif

int main()
{
    using namespace castle::io::uart;
    uart_config config("/dev/ttyS0", 115200U, data_bits::eight,
                       parity::none, stop_bits::one, flow_control::none,
                       0U, 0U, true);
    assert(validate_config(config) == castle::status::ok);

#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    // A pseudo-terminal exercises open/configure/read/write without UART hardware.
    const int master = ::posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert(master >= 0);
    assert(::grantpt(master) == 0);
    assert(::unlockpt(master) == 0);
    char* slave_path = ::ptsname(master);
    assert(slave_path != nullptr);

    config = uart_config(slave_path, 115200U, data_bits::eight,
                         parity::none, stop_bits::one, flow_control::none,
                         0U, 0U, true);
    basic_uart serial;
    assert(serial.open(config) == castle::status::ok);
    assert(serial.is_open() && serial.is_configured());
    assert(serial.configure(config) == castle::status::ok);
    assert(serial.flush_input() == castle::status::ok);
    assert(serial.flush_output() == castle::status::ok);
    assert(serial.flush() == castle::status::ok);

    const char tx[] = {'U', 'A', 'R', 'T'};
    assert(::write(master, tx, sizeof(tx)) == static_cast<ssize_t>(sizeof(tx)));
    char rx[sizeof(tx)] = {};
    castle::size_type received = 0U;
    assert(serial.read_some(rx, sizeof(rx), received) == castle::status::ok);
    assert(received == sizeof(rx) && rx[0] == 'U' && rx[3] == 'T');

    castle::size_type pending = 0U;
    assert(serial.bytes_available(pending) == castle::status::ok);
    assert(serial.write_all(tx, sizeof(tx), received) == castle::status::ok);
    assert(received == sizeof(tx));
    assert(serial.drain() == castle::status::ok);
    assert(serial.close() == castle::status::ok);
    assert(::close(master) == 0);
#else
    basic_uart serial;
    assert(serial.open(config) == castle::status::not_configured);
#endif
    return 0;
}
