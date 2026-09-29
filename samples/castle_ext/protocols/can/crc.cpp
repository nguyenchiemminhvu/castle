#include <assert.h>
#include <stdint.h>

#include "castle_ext/protocols/can/crc.hpp"

int main()
{
    using namespace castle::protocols::can;

    const uint8_t classical_payload[2U] = {0x11U, 0x22U};
    frame classical{};
    assert(castle::succeeded(make_data_frame(0x123U, identifier_format::standard,
        frame_format::classical, classical_payload, 2U, classical)));
    crc_result classical_crc{};
    assert(castle::succeeded(calculate_crc(classical, classical_crc)));
    assert(classical_crc.width == CAN_CLASSICAL_CRC_WIDTH);
    assert(classical_crc.value <= 0x7FFFU);

    uint8_t fd_payload[20U] = {};
    frame fd{};
    assert(castle::succeeded(make_data_frame(0x1ABCDEU, identifier_format::extended,
        frame_format::fd, fd_payload, 20U, fd, true, false)));
    crc_result fd_crc{};
    assert(castle::succeeded(calculate_crc(fd, fd_crc)));
    assert(fd_crc.width == CAN_FD_CRC21_WIDTH);
    assert(fd_crc.value <= 0x1FFFFFU);

    frame signal{};
    assert(castle::succeeded(make_error_frame(0U, identifier_format::standard, signal)));
    assert(calculate_crc(signal, fd_crc) == castle::status::invalid_argument);
    return 0;
}
