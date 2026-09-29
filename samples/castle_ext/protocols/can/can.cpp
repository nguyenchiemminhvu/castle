#include <assert.h>
#include <stdint.h>

#include "castle_ext/protocols/can/can.hpp"

int main()
{
    using namespace castle::protocols::can;
    frame value{};
    assert(castle::succeeded(make_data_frame(0x45U, identifier_format::standard,
        frame_format::classical, nullptr, 0U, value)));
    crc_result check{};
    assert(castle::succeeded(calculate_crc(value, check)));
    assert(check.width == CAN_CLASSICAL_CRC_WIDTH);
    assert(value.valid());
    return 0;
}
