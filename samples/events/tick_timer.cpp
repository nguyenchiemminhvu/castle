#include "sample_support.hpp"

#include "castle/events/tick_timer.hpp"

#include <stdint.h>

int main()
{
    using timer_type = castle::events::tick_timer<2U, 64U>;

    static_assert(timer_type::callback_capacity() == 2U, "Unexpected callback capacity");
    static_assert(timer_type::max_period() > 0U, "Unexpected maximum period");

    timer_type timer;
    const timer_type& const_timer = timer;
    uint32_t fired = 0U;
    timer_type::error error = timer_type::error::unknown_error;

    CASTLE_SAMPLE_CHECK(timer.callback_count() == 0U);
    CASTLE_SAMPLE_CHECK(timer.period() == 0U);
    CASTLE_SAMPLE_CHECK(timer.elapsed() == 0U);
    CASTLE_SAMPLE_CHECK(timer.remaining() == 0U);
    CASTLE_SAMPLE_CHECK(!timer.is_running());
    CASTLE_SAMPLE_CHECK(timer.current_mode() == timer_type::mode::periodic);
    CASTLE_SAMPLE_CHECK(timer.repeats_remaining() == 0U);
    CASTLE_SAMPLE_CHECK(timer.resume() == timer_type::error::not_configured);

    CASTLE_SAMPLE_CHECK(timer.set_period(3U) == timer_type::error::ok);
    CASTLE_SAMPLE_CHECK(timer.period() == 3U);

    auto first_subscription = timer.register_callback(
        [&fired]()
        {
            fired += 1U;
        },
        &error
    );
    CASTLE_SAMPLE_CHECK(error == timer_type::error::ok);
    CASTLE_SAMPLE_CHECK(static_cast<bool>(first_subscription));

    auto second_subscription = timer.register_callback(
        [&fired]()
        {
            fired += 10U;
        },
        &error
    );
    CASTLE_SAMPLE_CHECK(error == timer_type::error::ok);
    CASTLE_SAMPLE_CHECK(static_cast<bool>(second_subscription));
    CASTLE_SAMPLE_CHECK(timer.callback_count() == 2U);
    CASTLE_SAMPLE_CHECK(const_timer.registry().size() == 2U);

    CASTLE_SAMPLE_CHECK(timer.start(timer_type::mode::periodic) == timer_type::error::ok);
    CASTLE_SAMPLE_CHECK(timer.is_running());
    CASTLE_SAMPLE_CHECK(timer.remaining() == 3U);

    timer.on_tick(2U);
    CASTLE_SAMPLE_CHECK(timer.elapsed() == 2U);
    CASTLE_SAMPLE_CHECK(timer.remaining() == 1U);
    CASTLE_SAMPLE_CHECK(fired == 0U);

    timer.pause();
    CASTLE_SAMPLE_CHECK(!timer.is_running());
    timer.on_tick(5U);
    CASTLE_SAMPLE_CHECK(timer.elapsed() == 2U);
    CASTLE_SAMPLE_CHECK(fired == 0U);

    CASTLE_SAMPLE_CHECK(timer.resume() == timer_type::error::ok);
    timer.on_tick(1U);
    CASTLE_SAMPLE_CHECK(fired == 11U);
    CASTLE_SAMPLE_CHECK(timer.elapsed() == 0U);
    CASTLE_SAMPLE_CHECK(timer.remaining() == 3U);

    timer.on_tick(1U);
    CASTLE_SAMPLE_CHECK(timer.elapsed() == 1U);
    timer.reset();
    CASTLE_SAMPLE_CHECK(timer.elapsed() == 0U);

    CASTLE_SAMPLE_CHECK(timer.start(timer_type::mode::n_repeat, 0U) == timer_type::error::invalid_config);
    CASTLE_SAMPLE_CHECK(timer.start(timer_type::mode::n_repeat, 2U) == timer_type::error::ok);
    CASTLE_SAMPLE_CHECK(timer.current_mode() == timer_type::mode::n_repeat);
    CASTLE_SAMPLE_CHECK(timer.repeats_remaining() == 2U);

    timer.on_tick(6U);
    CASTLE_SAMPLE_CHECK(fired == 33U);
    CASTLE_SAMPLE_CHECK(!timer.is_running());
    CASTLE_SAMPLE_CHECK(timer.elapsed() == 0U);
    CASTLE_SAMPLE_CHECK(timer.remaining() == 0U);
    CASTLE_SAMPLE_CHECK(timer.repeats_remaining() == 0U);

    CASTLE_SAMPLE_CHECK(timer.start(timer_type::mode::one_shot) == timer_type::error::ok);
    CASTLE_SAMPLE_CHECK(timer.current_mode() == timer_type::mode::one_shot);
    timer.stop();
    CASTLE_SAMPLE_CHECK(!timer.is_running());
    CASTLE_SAMPLE_CHECK(timer.elapsed() == 0U);
    CASTLE_SAMPLE_CHECK(timer.repeats_remaining() == 0U);

    timer.clear_callbacks();
    CASTLE_SAMPLE_CHECK(timer.callback_count() == 0U);
    CASTLE_SAMPLE_CHECK(first_subscription.unsubscribe() == castle::status::invalid_subscription);
    CASTLE_SAMPLE_CHECK(second_subscription.unsubscribe() == castle::status::invalid_subscription);

    return 0;
}
