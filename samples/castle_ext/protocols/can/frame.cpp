#include <assert.h>
#include <stdint.h>

#include "castle_ext/protocols/can/frame.hpp"

int main()
{
    using namespace castle::protocols::can;

    const uint8_t payload[3U] = {0x12U, 0x34U, 0x56U};
    frame data{};
    assert(castle::succeeded(make_data_frame(
        0x123U, identifier_format::standard, frame_format::classical,
        payload, 3U, data)));
    assert(data.valid());
    assert(data.has_data());
    assert(!data.is_fd());
    assert(data.data_length_code() == 3U);
    assert(data.data[2U] == 0x56U);

    frame extended{};
    assert(castle::succeeded(make_data_frame(
        CAN_EXTENDED_IDENTIFIER_MAX, identifier_format::extended, frame_format::fd,
        payload, 3U, extended, true, true)));
    assert(extended.valid());
    assert(extended.is_fd());
    assert(extended.data_length_code() == 3U);
    assert(extended.bit_rate_switch && extended.error_state_indicator);

    frame remote{};
    assert(castle::succeeded(make_remote_frame(0x321U, identifier_format::standard, 8U, remote)));
    assert(remote.valid() && remote.kind == frame_kind::remote);
    assert(remote.data_length_code() == 8U);

    frame error{};
    assert(castle::succeeded(make_error_frame(0U, identifier_format::standard, error)));
    assert(error.valid() && error.kind == frame_kind::error);
    frame overload{};
    assert(castle::succeeded(make_overload_frame(0U, identifier_format::standard, overload)));
    assert(overload.valid() && overload.kind == frame_kind::overload);

    assert(!is_valid_fd_data_length(9U));
    assert(fd_data_length_code(64U) == 15U);
    assert(fd_data_length_code(9U) == 0xFFU);

    frame invalid{};
    assert(make_data_frame(0x800U, identifier_format::standard, frame_format::classical,
        payload, 3U, invalid) == castle::status::invalid_argument);
    return 0;
}
