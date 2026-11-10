#include <gtest/gtest.h>

#include "castle/patterns/action_sequence.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/chrono/duration.hpp"

#include <stdint.h>

namespace dp = castle::patterns;

namespace
{
using state = dp::action_sequence_state;
using ms = castle::chrono::milliseconds;
using ms8 = castle::chrono::duration<uint8_t, castle::milli>;

struct recorder
{
    int order[16] = {};
    int count = 0;

    void add(int id) noexcept
    {
        order[count++] = id;
    }
};

TEST(ActionSequenceTest, NewSequenceIsEmptyAndIdle)
{
    dp::action_sequence<3U> sequence;
    EXPECT_TRUE(sequence.empty());
    EXPECT_EQ(sequence.size(), 0U);
    EXPECT_EQ(sequence.state(), state::idle);
    EXPECT_FALSE(sequence.is_running());
    EXPECT_FALSE(sequence.is_completed());
    EXPECT_EQ(sequence.next_index(), 0U);
    EXPECT_EQ(sequence.remaining_delay(), ms(0));
    EXPECT_EQ((dp::action_sequence<3U>::capacity()), 3U);
}

TEST(ActionSequenceTest, UpdateBeforeStartDoesNothing)
{
    recorder log;
    dp::action_sequence<2U> sequence;
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    EXPECT_EQ(sequence.update(ms(5)), state::idle);
    EXPECT_EQ(log.count, 0);
}

TEST(ActionSequenceTest, EmptySequenceCompletesOnStart)
{
    dp::action_sequence<2U> sequence;
    EXPECT_EQ(sequence.start(), castle::status::ok);
    EXPECT_EQ(sequence.state(), state::completed);
    EXPECT_EQ(sequence.update(), state::completed);
}

TEST(ActionSequenceTest, StartDoesNotExecuteAnyStep)
{
    recorder log;
    dp::action_sequence<2U> sequence;
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);
    EXPECT_EQ(log.count, 0);
    EXPECT_TRUE(sequence.is_running());
}

TEST(ActionSequenceTest, CallsRunInOrderAndTakeNoTime)
{
    recorder log;
    dp::action_sequence<3U> sequence;
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(2); }), castle::status::ok);
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(3); }), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);

    EXPECT_EQ(sequence.update(), state::completed);
    ASSERT_EQ(log.count, 3);
    EXPECT_EQ(log.order[0], 1);
    EXPECT_EQ(log.order[1], 2);
    EXPECT_EQ(log.order[2], 3);
    EXPECT_TRUE(sequence.is_completed());
}

TEST(ActionSequenceTest, DelayHoldsLaterStepsUntilEnoughTicksElapse)
{
    recorder log;
    dp::action_sequence<3U> sequence;
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(sequence.add_delay(ms(10)), castle::status::ok);
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(2); }), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);

    EXPECT_EQ(sequence.update(), state::running);
    EXPECT_EQ(log.count, 1);
    EXPECT_EQ(sequence.next_index(), 1U);
    EXPECT_EQ(sequence.remaining_delay(), ms(10));

    EXPECT_EQ(sequence.update(ms(4)), state::running);
    EXPECT_EQ(sequence.remaining_delay(), ms(6));
    EXPECT_EQ(log.count, 1);

    EXPECT_EQ(sequence.update(ms(5)), state::running);
    EXPECT_EQ(sequence.remaining_delay(), ms(1));

    EXPECT_EQ(sequence.update(ms(1)), state::completed);
    ASSERT_EQ(log.count, 2);
    EXPECT_EQ(log.order[1], 2);
    EXPECT_EQ(sequence.remaining_delay(), ms(0));
}

TEST(ActionSequenceTest, ExcessTicksCarryIntoFollowingDelays)
{
    recorder log;
    dp::action_sequence<5U> sequence;
    ASSERT_EQ(sequence.add_delay(ms(3)), castle::status::ok);
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(sequence.add_delay(ms(4)), castle::status::ok);
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(2); }), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);

    EXPECT_EQ(sequence.update(ms(5)), state::running);
    EXPECT_EQ(log.count, 1);
    EXPECT_EQ(sequence.remaining_delay(), ms(2));

    EXPECT_EQ(sequence.update(ms(100)), state::completed);
    EXPECT_EQ(log.count, 2);
}

TEST(ActionSequenceTest, ZeroLengthDelayCompletesWithoutTicks)
{
    recorder log;
    dp::action_sequence<3U> sequence;
    ASSERT_EQ(sequence.add_delay(ms(0)), castle::status::ok);
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);
    EXPECT_EQ(sequence.update(), state::completed);
    EXPECT_EQ(log.count, 1);
}

TEST(ActionSequenceTest, RestartRunsEverythingAgainAndRewindsDelays)
{
    recorder log;
    dp::action_sequence<3U, ms8> sequence;
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(sequence.add_delay(ms8(20U)), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);
    EXPECT_EQ(sequence.update(ms8(255U)), state::completed);

    ASSERT_EQ(sequence.start(), castle::status::ok);
    EXPECT_EQ(sequence.update(), state::running);
    EXPECT_EQ(sequence.remaining_delay(), ms(20));
    EXPECT_EQ(log.count, 2);

    // Restart in the middle of a delay also rewinds it.
    EXPECT_EQ(sequence.update(ms8(15U)), state::running);
    ASSERT_EQ(sequence.start(), castle::status::ok);
    EXPECT_EQ(sequence.update(), state::running);
    EXPECT_EQ(sequence.remaining_delay(), ms(20));
}

TEST(ActionSequenceTest, CapacityIsEnforced)
{
    dp::action_sequence<2U> sequence;
    EXPECT_EQ(sequence.add_delay(ms(1)), castle::status::ok);
    EXPECT_EQ(sequence.add_call([]() noexcept {}), castle::status::ok);
    EXPECT_EQ(sequence.add_call([]() noexcept {}), castle::status::full);
    EXPECT_EQ(sequence.add_delay(ms(1)), castle::status::full);
    EXPECT_EQ(sequence.size(), 2U);
}

void plain_function() noexcept {}

TEST(ActionSequenceTest, FunctionPointersAreAcceptedAndNullIsRejected)
{
    dp::action_sequence<2U> sequence;
    void (*null_function)() = nullptr;
    EXPECT_EQ(sequence.add_call(&plain_function), castle::status::ok);
    EXPECT_EQ(sequence.add_call(null_function), castle::status::invalid_callback);
    EXPECT_EQ(sequence.size(), 1U);
}

TEST(ActionSequenceTest, EditingIsRejectedWhileRunning)
{
    dp::action_sequence<4U> sequence;
    dp::action_sequence<2U> other;
    ASSERT_EQ(sequence.add_delay(ms(5)), castle::status::ok);
    ASSERT_EQ(other.add_delay(ms(1)), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);

    EXPECT_EQ(sequence.add_call([]() noexcept {}), castle::status::invalid_config);
    EXPECT_EQ(sequence.add_delay(ms(1)), castle::status::invalid_config);
    EXPECT_EQ(sequence.append(other), castle::status::invalid_config);
    EXPECT_EQ(sequence.size(), 1U);

    // A running source cannot be appended either.
    dp::action_sequence<4U> destination;
    ASSERT_EQ(other.start(), castle::status::ok);
    EXPECT_EQ(destination.append(other), castle::status::invalid_config);
    EXPECT_TRUE(destination.empty());
}

TEST(ActionSequenceTest, EditingAfterCompletionRewindsToIdle)
{
    recorder log;
    dp::action_sequence<3U> sequence;
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);
    ASSERT_EQ(sequence.update(), state::completed);

    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(2); }), castle::status::ok);
    EXPECT_EQ(sequence.state(), state::idle);
    EXPECT_EQ(sequence.next_index(), 0U);
}

TEST(ActionSequenceTest, AppendCopiesStepsAndLeavesSourceIntact)
{
    recorder log;
    dp::action_sequence<2U> tail;
    ASSERT_EQ(tail.add_delay(ms(2)), castle::status::ok);
    ASSERT_EQ(tail.add_call([&log]() noexcept { log.add(2); }), castle::status::ok);

    dp::action_sequence<4U> head;
    ASSERT_EQ(head.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(head.append(tail), castle::status::ok);
    EXPECT_EQ(head.size(), 3U);
    EXPECT_EQ(tail.size(), 2U);

    ASSERT_EQ(head.start(), castle::status::ok);
    EXPECT_EQ(head.update(), state::running);
    EXPECT_EQ(head.remaining_delay(), ms(2));
    EXPECT_EQ(head.update(ms(2)), state::completed);
    ASSERT_EQ(log.count, 2);
    EXPECT_EQ(log.order[0], 1);
    EXPECT_EQ(log.order[1], 2);

    // The source is independent and still runnable.
    ASSERT_EQ(tail.start(), castle::status::ok);
    EXPECT_EQ(tail.update(ms(2)), state::completed);
    EXPECT_EQ(log.count, 3);
}

TEST(ActionSequenceTest, AppendThatDoesNotFitChangesNothing)
{
    dp::action_sequence<3U> big;
    ASSERT_EQ(big.add_delay(ms(1)), castle::status::ok);
    ASSERT_EQ(big.add_delay(ms(1)), castle::status::ok);
    ASSERT_EQ(big.add_delay(ms(1)), castle::status::ok);

    dp::action_sequence<3U> destination;
    ASSERT_EQ(destination.add_delay(ms(1)), castle::status::ok);
    EXPECT_EQ(destination.append(big), castle::status::full);
    EXPECT_EQ(destination.size(), 1U);
}

TEST(ActionSequenceTest, AppendOfPartiallyRunDelayStartsFromFullDuration)
{
    dp::action_sequence<2U> source;
    ASSERT_EQ(source.add_delay(ms(10)), castle::status::ok);
    ASSERT_EQ(source.start(), castle::status::ok);
    source.update(ms(4));
    ASSERT_EQ(source.reset(), castle::status::ok);

    dp::action_sequence<2U> destination;
    ASSERT_EQ(destination.append(source), castle::status::ok);
    ASSERT_EQ(destination.start(), castle::status::ok);
    destination.update();
    EXPECT_EQ(destination.remaining_delay(), ms(10));
}

TEST(ActionSequenceTest, ResetReturnsToIdleAndKeepsSteps)
{
    recorder log;
    dp::action_sequence<2U> sequence;
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(sequence.add_delay(ms(5)), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);
    sequence.update(ms(1));

    EXPECT_EQ(sequence.reset(), castle::status::ok);
    EXPECT_EQ(sequence.state(), state::idle);
    EXPECT_EQ(sequence.size(), 2U);
    EXPECT_EQ(sequence.next_index(), 0U);

    ASSERT_EQ(sequence.start(), castle::status::ok);
    sequence.update(ms(1));
    EXPECT_EQ(sequence.remaining_delay(), ms(4));
}

TEST(ActionSequenceTest, ClearRemovesStepsAndReleasesCallbacks)
{
    int destroyed = 0;
    struct tracker
    {
        int* counter;
        explicit tracker(int* value) noexcept : counter(value) {}
        tracker(tracker const& other) noexcept : counter(other.counter) {}
        ~tracker() { ++(*counter); }
        void operator()() const noexcept {}
    };

    dp::action_sequence<2U> sequence;
    ASSERT_EQ(sequence.add_call(tracker{&destroyed}), castle::status::ok);
    const int before_clear = destroyed;
    EXPECT_EQ(sequence.clear(), castle::status::ok);
    EXPECT_GT(destroyed, before_clear);
    EXPECT_TRUE(sequence.empty());
    EXPECT_EQ(sequence.state(), state::idle);
    EXPECT_EQ(sequence.add_delay(ms(1)), castle::status::ok);
}

TEST(ActionSequenceTest, StepsCannotRestructureTheSequenceThatRunsThem)
{
    dp::action_sequence<3U> sequence;
    castle::status results[6] = {};
    state update_state_inside = state::idle;
    dp::action_sequence<1U> other;

    ASSERT_EQ(sequence.add_call(
                  [&]() noexcept
                  {
                      results[0] = sequence.start();
                      results[1] = sequence.reset();
                      results[2] = sequence.clear();
                      results[3] = sequence.add_delay(ms(1));
                      results[4] = sequence.append(other);
                      update_state_inside = sequence.update();
                  }),
              castle::status::ok);

    ASSERT_EQ(sequence.start(), castle::status::ok);
    EXPECT_EQ(sequence.update(), state::completed);
    EXPECT_EQ(results[0], castle::status::invalid_config);
    EXPECT_EQ(results[1], castle::status::invalid_config);
    EXPECT_EQ(results[2], castle::status::invalid_config);
    EXPECT_EQ(results[3], castle::status::invalid_config);
    EXPECT_EQ(results[4], castle::status::invalid_config);
    EXPECT_EQ(update_state_inside, state::running);
    EXPECT_EQ(sequence.size(), 1U);
}

TEST(ActionSequenceTest, CopyOwnsIndependentCallbacksAndProgress)
{
    int calls = 0;
    dp::action_sequence<2U> original;
    ASSERT_EQ(original.add_call([&calls]() noexcept { ++calls; }), castle::status::ok);
    ASSERT_EQ(original.add_delay(ms(3)), castle::status::ok);

    dp::action_sequence<2U> copy = original;
    ASSERT_EQ(copy.start(), castle::status::ok);
    EXPECT_EQ(copy.update(ms(3)), state::completed);
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(original.state(), state::idle);
}
TEST(ActionSequenceTest, CoarserUnitsConvertExactly)
{
    recorder log;
    dp::action_sequence<2U> sequence;
    ASSERT_EQ(sequence.add_delay(castle::chrono::seconds(2)), castle::status::ok);
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);

    EXPECT_EQ(sequence.update(castle::chrono::seconds(1)), state::running);
    EXPECT_EQ(sequence.remaining_delay(), ms(1000));
    EXPECT_EQ(sequence.update(ms(999)), state::running);
    EXPECT_EQ(log.count, 0);
    EXPECT_EQ(sequence.update(ms(1)), state::completed);
    EXPECT_EQ(log.count, 1);
}

TEST(ActionSequenceTest, NegativeDelayIsRejected)
{
    dp::action_sequence<2U> sequence;
    EXPECT_EQ(sequence.add_delay(ms(-1)), castle::status::invalid_argument);
    EXPECT_TRUE(sequence.empty());
}

TEST(ActionSequenceTest, DelayBeyondDurationRangeIsRejected)
{
    dp::action_sequence<2U, ms8> sequence;
    EXPECT_EQ(sequence.add_delay(ms(255)), castle::status::ok);
    EXPECT_EQ(sequence.add_delay(ms(256)), castle::status::out_of_range);
    EXPECT_EQ(sequence.add_delay(castle::chrono::seconds(1)), castle::status::out_of_range);
    EXPECT_EQ(sequence.size(), 1U);
}

TEST(ActionSequenceTest, NegativeElapsedCountsAsZero)
{
    dp::action_sequence<1U> sequence;
    ASSERT_EQ(sequence.add_delay(ms(5)), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);
    EXPECT_EQ(sequence.update(ms(-3)), state::running);
    EXPECT_EQ(sequence.remaining_delay(), ms(5));
}

TEST(ActionSequenceTest, ElapsedBeyondDurationRangeSaturates)
{
    recorder log;
    dp::action_sequence<3U, ms8> sequence;
    ASSERT_EQ(sequence.add_delay(ms8(200U)), castle::status::ok);
    ASSERT_EQ(sequence.add_call([&log]() noexcept { log.add(1); }), castle::status::ok);
    ASSERT_EQ(sequence.add_delay(ms8(100U)), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);

    // 1000 ms does not fit uint8_t; it saturates to 255 ms: the first delay ends, 55 ms carry over.
    EXPECT_EQ(sequence.update(ms(1000)), state::running);
    EXPECT_EQ(log.count, 1);
    EXPECT_EQ(sequence.remaining_delay(), ms8(45U));
}

TEST(ActionSequenceTest, NanosecondSequenceAcceptsCoarserClockDifferences)
{
    using ns = castle::chrono::nanoseconds;
    dp::action_sequence<1U, ns> sequence;
    ASSERT_EQ(sequence.add_delay(ms(1)), castle::status::ok);
    ASSERT_EQ(sequence.start(), castle::status::ok);
    EXPECT_EQ(sequence.update(castle::chrono::microseconds(400)), state::running);
    EXPECT_EQ(sequence.remaining_delay(), ns(600000));
    EXPECT_EQ(sequence.update(ns(600000)), state::completed);
}

TEST(ActionSequenceTest, EmptyFunctionWrapperIsRejected)
{
    dp::action_sequence<2U> sequence;
    castle::callbacks::function<void(), 16U> empty_small;
    castle::callbacks::function<void()> empty_same;
    castle::callbacks::function<void(), 16U> filled{[]() noexcept {}};
    EXPECT_EQ(sequence.add_call(empty_small), castle::status::invalid_callback);
    EXPECT_EQ(sequence.add_call(empty_same), castle::status::invalid_callback);
    EXPECT_EQ(sequence.add_call(filled), castle::status::ok);
    EXPECT_EQ(sequence.size(), 1U);
}

TEST(ActionSequenceTest, RemainingDelayIsZeroWhenNotRunningOrNextIsCall)
{
    dp::action_sequence<2U> sequence;
    ASSERT_EQ(sequence.add_delay(ms(5)), castle::status::ok);
    ASSERT_EQ(sequence.add_call([]() noexcept {}), castle::status::ok);
    EXPECT_EQ(sequence.remaining_delay(), ms(0));
    ASSERT_EQ(sequence.start(), castle::status::ok);
    EXPECT_EQ(sequence.remaining_delay(), ms(5));
    EXPECT_EQ(sequence.update(ms(5)), state::completed);
    EXPECT_EQ(sequence.remaining_delay(), ms(0));
}
} // namespace
