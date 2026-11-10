#include "sample_support.hpp"

#include "castle/patterns/action_sequence.hpp"
#include "castle/chrono/duration.hpp"

#include <stdint.h>

namespace dp = castle::patterns;
using castle::chrono::milliseconds;
using castle::chrono::seconds;

namespace
{
struct door_log
{
    uint32_t opened = 0U;
    uint32_t closed = 0U;
    uint32_t locked = 0U;
};
} // namespace

int main()
{
    door_log log;

    // Equivalent of cocos2d-x: Sequence::create(CallFunc(open), DelayTime(0.1f), CallFunc(close), nullptr)
    // The default resolution is milliseconds; coarser units such as seconds convert exactly.
    dp::action_sequence<6U> close_door;
    CASTLE_SAMPLE_CHECK(close_door.add_call([&log]() noexcept { ++log.opened; }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(close_door.add_delay(milliseconds(100)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(close_door.add_call([&log]() noexcept { ++log.closed; }) == castle::status::ok);

    // Composition: a second sequence is copied (flattened) after the first.
    dp::action_sequence<2U> lock_step;
    CASTLE_SAMPLE_CHECK(lock_step.add_delay(milliseconds(50)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(lock_step.add_call([&log]() noexcept { ++log.locked; }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(close_door.append(lock_step) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(close_door.size() == 5U);

    // The capacity is a compile-time bound: a sixth step is rejected.
    CASTLE_SAMPLE_CHECK(close_door.add_delay(milliseconds(1)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(close_door.add_delay(milliseconds(1)) == castle::status::full);

    CASTLE_SAMPLE_CHECK(close_door.start() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(close_door.update() == dp::action_sequence_state::running);
    CASTLE_SAMPLE_CHECK(log.opened == 1U && log.closed == 0U);

    CASTLE_SAMPLE_CHECK(close_door.update(milliseconds(60)) == dp::action_sequence_state::running);
    CASTLE_SAMPLE_CHECK(close_door.remaining_delay() == milliseconds(40));
    CASTLE_SAMPLE_CHECK(log.closed == 0U);

    // 40 ticks end the first delay; the remaining 30 carry through the close call into the 50-tick delay.
    CASTLE_SAMPLE_CHECK(close_door.update(milliseconds(70)) == dp::action_sequence_state::running);
    CASTLE_SAMPLE_CHECK(log.closed == 1U && log.locked == 0U);
    CASTLE_SAMPLE_CHECK(close_door.remaining_delay() == milliseconds(20));

    CASTLE_SAMPLE_CHECK(close_door.update(milliseconds(20)) == dp::action_sequence_state::running);
    CASTLE_SAMPLE_CHECK(log.locked == 1U);
    CASTLE_SAMPLE_CHECK(close_door.update(milliseconds(1)) == dp::action_sequence_state::completed);
    CASTLE_SAMPLE_CHECK(close_door.is_completed());

    // The sequence owns its steps, so it can run again after start().
    CASTLE_SAMPLE_CHECK(close_door.start() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(close_door.update(seconds(1)) == dp::action_sequence_state::completed);
    CASTLE_SAMPLE_CHECK(log.opened == 2U && log.closed == 2U && log.locked == 2U);

    return 0;
}
