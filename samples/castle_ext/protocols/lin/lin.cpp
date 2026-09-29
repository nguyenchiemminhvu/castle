#include <assert.h>
#include <stdint.h>

#include "castle_ext/protocols/lin/lin.hpp"

int main()
{
    using namespace castle::protocols::lin;
    const uint8_t data[1U] = {0xA5U};
    frame response{};
    assert(castle::succeeded(make_frame(0x22U, data, 1U,
        checksum_type::enhanced, response)));
    assert(validate_checksum(response));

    schedule_table<3U> schedule;
    assert(castle::succeeded(schedule.add(response.identifier, response_owner::slave, 10U)));
    schedule_entry slot{};
    assert(castle::succeeded(schedule.next(slot)));
    assert(slot.identifier == response.identifier);
    return 0;
}
