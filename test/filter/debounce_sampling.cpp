#include <gtest/gtest.h>
#include "castle/filter/debounce_sampling.hpp"

namespace
{

using castle::filter::debounce_sampling;
using castle::filter::debounce_state;

TEST(DebounceSamplingTest, InitialStateIsClearedAndNotValid)
{
    debounce_sampling<3U> d;
    EXPECT_EQ(d.state(), debounce_state::cleared);
    EXPECT_FALSE(d.is_valid());
    EXPECT_FALSE(d.is_held());
    EXPECT_FALSE(d.is_repeating());
    EXPECT_FALSE(d.value());
}

TEST(DebounceSamplingTest, ValidTransitionRequiresExactValidCountSamplesOffByOne)
{
    debounce_sampling<4U> d;

    EXPECT_FALSE(d.sample(true));  // 1
    EXPECT_FALSE(d.sample(true));  // 2
    EXPECT_FALSE(d.sample(true));  // 3
    EXPECT_FALSE(d.is_valid());
    EXPECT_TRUE(d.sample(true));   // 4 -> valid
    EXPECT_TRUE(d.is_valid());
}

TEST(DebounceSamplingTest, ValidCountOfOneTriggersOnFirstSample)
{
    debounce_sampling<1U> d;
    EXPECT_TRUE(d.sample(true));
    EXPECT_TRUE(d.is_valid());

    EXPECT_TRUE(d.sample(false));
    EXPECT_FALSE(d.is_valid());
}

TEST(DebounceSamplingTest, ReleaseUsesSameValidCountDebounceDelay)
{
    debounce_sampling<3U> d;
    for (uint32_t i = 0U; i < 3U; ++i)
    {
        d.sample(true);
    }
    ASSERT_TRUE(d.is_valid());

    EXPECT_FALSE(d.sample(false)); // 1
    EXPECT_FALSE(d.sample(false)); // 2
    EXPECT_TRUE(d.is_valid());     // still valid: release not yet confirmed
    EXPECT_TRUE(d.sample(false));  // 3 -> cleared
    EXPECT_FALSE(d.is_valid());
}

TEST(DebounceSamplingTest, HoldTransitionAfterHoldCountSincoValid)
{
    debounce_sampling<2U, 3U> d;

    d.sample(true);
    ASSERT_TRUE(d.sample(true)); // valid at count 2
    ASSERT_TRUE(d.is_valid());
    ASSERT_FALSE(d.is_held());

    EXPECT_FALSE(d.sample(true)); // 1 since valid
    EXPECT_FALSE(d.sample(true)); // 2 since valid
    EXPECT_FALSE(d.is_held());
    EXPECT_TRUE(d.sample(true));  // 3 since valid -> held
    EXPECT_TRUE(d.is_held());
    EXPECT_TRUE(d.is_valid());
}

TEST(DebounceSamplingTest, RepeatTransitionAfterRepeatCountSinceHeld)
{
    debounce_sampling<2U, 2U, 3U> d;

    for (uint32_t i = 0U; i < 4U; ++i)
    {
        d.sample(true); // 2 to become valid, 2 more to become held
    }
    ASSERT_TRUE(d.is_held());
    ASSERT_FALSE(d.is_repeating());

    EXPECT_FALSE(d.sample(true)); // 1 since held
    EXPECT_FALSE(d.sample(true)); // 2 since held
    EXPECT_TRUE(d.sample(true));  // 3 since held -> repeating
    EXPECT_TRUE(d.is_repeating());
}

TEST(DebounceSamplingTest, RepeatingSelfLoopFiresPeriodically)
{
    debounce_sampling<1U, 1U, 2U> d;

    ASSERT_TRUE(d.sample(true));  // valid
    ASSERT_TRUE(d.sample(true));  // held
    EXPECT_FALSE(d.sample(true)); // 1 since held
    ASSERT_TRUE(d.sample(true));  // 2 since held -> repeating (1st fire)
    ASSERT_TRUE(d.is_repeating());

    EXPECT_FALSE(d.sample(true)); // 1 since last repeat
    EXPECT_TRUE(d.sample(true));  // 2 since last repeat -> re-fires
    EXPECT_TRUE(d.is_repeating());
    EXPECT_FALSE(d.sample(true));
    EXPECT_TRUE(d.sample(true));  // fires again
}

TEST(DebounceSamplingTest, BouncingInputResetsCounterWithoutStateTransition)
{
    debounce_sampling<5U> d;

    EXPECT_FALSE(d.sample(true));
    EXPECT_FALSE(d.sample(true));
    EXPECT_FALSE(d.sample(true));
    EXPECT_FALSE(d.sample(false)); // bounce: counter restarts
    EXPECT_FALSE(d.is_valid());

    EXPECT_FALSE(d.sample(true)); // counter restarts again
    EXPECT_FALSE(d.sample(true));
    EXPECT_FALSE(d.sample(true));
    EXPECT_FALSE(d.sample(true));
    EXPECT_TRUE(d.sample(true));  // 5 consecutive since last bounce -> valid
    EXPECT_TRUE(d.is_valid());
}

TEST(DebounceSamplingTest, RapidTogglingNeverReachesValid)
{
    debounce_sampling<10U> d;
    bool sample_value = true;
    for (uint32_t i = 0U; i < 100U; ++i)
    {
        EXPECT_FALSE(d.sample(sample_value));
        sample_value = !sample_value;
    }
    EXPECT_FALSE(d.is_valid());
}

TEST(DebounceSamplingTest, ResetRestoresClearedStateAndCounter)
{
    debounce_sampling<3U> d;
    for (uint32_t i = 0U; i < 3U; ++i)
    {
        d.sample(true);
    }
    ASSERT_TRUE(d.is_valid());

    d.reset();
    EXPECT_EQ(d.state(), debounce_state::cleared);
    EXPECT_FALSE(d.is_valid());
    EXPECT_FALSE(d.value());

    // Counter must also have restarted: 2 samples are not enough post-reset.
    EXPECT_FALSE(d.sample(true));
    EXPECT_FALSE(d.sample(true));
    EXPECT_TRUE(d.sample(true));
}

TEST(DebounceSamplingTest, ResetWithInitialSampleSeedsBaseline)
{
    debounce_sampling<2U> d;
    d.reset(true);
    EXPECT_EQ(d.state(), debounce_state::cleared);
    EXPECT_TRUE(d.value());

    // Sampling the same value as the seeded baseline continues the run
    // immediately rather than restarting the counter.
    EXPECT_FALSE(d.sample(true)); // count = 1
    EXPECT_TRUE(d.sample(true));  // count = 2 -> valid
}

TEST(DebounceSamplingTest, ConstructorInitialSampleSeedsBaseline)
{
    debounce_sampling<2U> d(true);
    EXPECT_TRUE(d.value());

    // Sampling "true" again matches the baseline and continues the run immediately.
    EXPECT_FALSE(d.sample(true)); // count = 1
    EXPECT_TRUE(d.sample(true));  // count = 2 -> valid
}

TEST(DebounceSamplingTest, CallbackInvokedOnEachTransitionType)
{
    debounce_sampling<1U, 1U, 1U> d;
    uint32_t valid_calls = 0U;
    uint32_t held_calls = 0U;
    uint32_t repeating_calls = 0U;
    uint32_t cleared_calls = 0U;

    d.on_valid([&valid_calls] { ++valid_calls; });
    d.on_held([&held_calls] { ++held_calls; });
    d.on_repeating([&repeating_calls] { ++repeating_calls; });
    d.on_cleared([&cleared_calls] { ++cleared_calls; });

    d.sample(true);  // valid
    d.sample(true);  // held
    d.sample(true);  // repeating
    EXPECT_EQ(valid_calls, 1U);
    EXPECT_EQ(held_calls, 1U);
    EXPECT_EQ(repeating_calls, 1U);
    EXPECT_EQ(cleared_calls, 0U);

    d.sample(false); // cleared
    EXPECT_EQ(cleared_calls, 1U);
}

TEST(DebounceSamplingTest, CallbackAbsenceDoesNotCrashAndStillTransitions)
{
    debounce_sampling<1U, 1U, 1U> d;

    EXPECT_TRUE(d.sample(true));
    EXPECT_TRUE(d.sample(true));
    EXPECT_TRUE(d.sample(true));
    EXPECT_TRUE(d.is_repeating());
    EXPECT_TRUE(d.sample(false));
    EXPECT_FALSE(d.is_valid());
}

TEST(DebounceSamplingTest, CallbackReplacementUsesLatestHandler)
{
    debounce_sampling<1U> d;
    uint32_t first_calls = 0U;
    uint32_t second_calls = 0U;

    d.on_valid([&first_calls] { ++first_calls; });
    d.on_valid([&second_calls] { ++second_calls; }); // replaces the first handler

    d.sample(true);
    EXPECT_EQ(first_calls, 0U);
    EXPECT_EQ(second_calls, 1U);
}

TEST(DebounceSamplingTest, CallbackRemovalStopsInvocation)
{
    using debounce_type = debounce_sampling<1U>;
    debounce_type d;
    uint32_t calls = 0U;

    d.on_valid([&calls] { ++calls; });
    d.sample(true);
    EXPECT_EQ(calls, 1U);

    d.reset(false);
    d.on_valid(debounce_type::callback_type{});
    d.sample(true);
    EXPECT_EQ(calls, 1U); // unchanged
}

TEST(DebounceSamplingTest, ClearCallbacksRemovesAllFourHandlers)
{
    debounce_sampling<1U, 1U, 1U> d;
    uint32_t calls = 0U;

    d.on_valid([&calls] { ++calls; });
    d.on_held([&calls] { ++calls; });
    d.on_repeating([&calls] { ++calls; });
    d.on_cleared([&calls] { ++calls; });
    d.clear_callbacks();

    d.sample(true);
    d.sample(true);
    d.sample(true);
    d.sample(false);
    EXPECT_EQ(calls, 0U);
}

TEST(DebounceSamplingTest, SampleReturnValueMatchesTransitionOccurrence)
{
    debounce_sampling<2U> d;
    EXPECT_FALSE(d.sample(true));  // 1
    EXPECT_TRUE(d.sample(true));   // 2 -> valid
    EXPECT_FALSE(d.sample(true));  // still valid, no change
    EXPECT_FALSE(d.sample(false)); // 1
    EXPECT_TRUE(d.sample(false));  // 2 -> cleared
}

TEST(DebounceSamplingTest, HoldDisabledWhenHoldCountIsZero)
{
    debounce_sampling<2U> d; // HoldCount defaults to 0 -> disabled
    for (uint32_t i = 0U; i < 1000U; ++i)
    {
        d.sample(true);
    }
    EXPECT_TRUE(d.is_valid());
    EXPECT_FALSE(d.is_held());
}

TEST(DebounceSamplingTest, RepeatDisabledWhenRepeatCountIsZero)
{
    debounce_sampling<2U, 2U> d; // RepeatCount defaults to 0 -> disabled
    for (uint32_t i = 0U; i < 1000U; ++i)
    {
        d.sample(true);
    }
    EXPECT_TRUE(d.is_held());
    EXPECT_FALSE(d.is_repeating());
}

TEST(DebounceSamplingTest, StateQueriesAreConsistentAcrossFullLifecycle)
{
    debounce_sampling<1U, 1U, 1U> d;

    d.sample(true); // valid
    EXPECT_TRUE(d.is_valid());
    EXPECT_FALSE(d.is_held());
    EXPECT_FALSE(d.is_repeating());

    d.sample(true); // held
    EXPECT_TRUE(d.is_valid());
    EXPECT_TRUE(d.is_held());
    EXPECT_FALSE(d.is_repeating());

    d.sample(true); // repeating
    EXPECT_TRUE(d.is_valid());
    EXPECT_TRUE(d.is_held());
    EXPECT_TRUE(d.is_repeating());

    d.sample(false); // cleared
    EXPECT_FALSE(d.is_valid());
    EXPECT_FALSE(d.is_held());
    EXPECT_FALSE(d.is_repeating());
}

TEST(DebounceSamplingTest, OperatorCallForwardsToSample)
{
    debounce_sampling<2U> d;
    EXPECT_FALSE(d(true));
    EXPECT_TRUE(d(true));
    EXPECT_TRUE(d.is_valid());
}

TEST(DebounceSamplingTest, ValueAccessorReflectsLastObservedSample)
{
    debounce_sampling<3U> d;
    EXPECT_FALSE(d.value());
    d.sample(true);
    EXPECT_TRUE(d.value());
    d.sample(false);
    EXPECT_FALSE(d.value());
}

TEST(DebounceSamplingTest, NarrowCounterTypeCompilesAndBehavesIdentically)
{
    debounce_sampling<5U, 5U, 5U, uint8_t> d;
    for (uint32_t i = 0U; i < 4U; ++i)
    {
        EXPECT_FALSE(d.sample(true));
    }
    EXPECT_TRUE(d.sample(true));
    EXPECT_TRUE(d.is_valid());
}

} // namespace
