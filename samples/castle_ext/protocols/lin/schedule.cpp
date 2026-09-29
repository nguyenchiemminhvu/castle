#include <assert.h>
#include <stdint.h>

#include "castle_ext/protocols/lin/schedule.hpp"

int main()
{
    using namespace castle::protocols::lin;
    schedule_table<2U> schedule;
    assert(schedule.empty());
    assert(schedule.capacity() == 2U);
    schedule_entry slot{};
    assert(schedule.next(slot) == castle::status::empty);
    assert(castle::succeeded(schedule.add(0x10U, response_owner::slave, 5U)));
    assert(castle::succeeded(schedule.add(0x11U, response_owner::master, 2U)));
    assert(schedule.add(0x12U, response_owner::slave) == castle::status::full);

    assert(castle::succeeded(schedule.get(0U, slot)));
    assert(slot.identifier == 0x10U && slot.slot_ticks == 5U);
    assert(schedule.get(9U, slot) == castle::status::out_of_range);
    assert(castle::succeeded(schedule.next(slot)) && slot.identifier == 0x10U);
    assert(castle::succeeded(schedule.next(slot)) && slot.identifier == 0x11U);
    assert(castle::succeeded(schedule.next(slot)) && slot.identifier == 0x10U);
    schedule.reset();
    assert(castle::succeeded(schedule.next(slot)) && slot.identifier == 0x10U);
    schedule.clear();
    assert(schedule.empty());
    return 0;
}
