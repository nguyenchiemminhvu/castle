#include "sample_support.hpp"

#include "castle/callbacks/exec_policy.hpp"

#include <stdint.h>

namespace
{
struct fake_clock
{
    using duration = castle::chrono::milliseconds;
    using time_point = castle::chrono::time_point<fake_clock, duration>;

    static time_point now() noexcept
    {
        return time_point(duration(now_ms));
    }

    static int64_t now_ms;
};

int64_t fake_clock::now_ms = 0;
} // namespace

int main()
{
    using namespace castle::chrono::literals::chrono_literals;
    namespace policy = castle::callbacks::policy;

    int hits = 0;
    policy::once once_policy{[&hits](int value) noexcept { hits += value; }};
    CASTLE_SAMPLE_CHECK(!once_policy.has_fired());
    once_policy.execute(2);
    once_policy(5);
    CASTLE_SAMPLE_CHECK(hits == 2);
    CASTLE_SAMPLE_CHECK(once_policy.has_fired());
    once_policy.reset();
    once_policy(3);
    CASTLE_SAMPLE_CHECK(hits == 5);

    auto concurrent_once = policy::make_once<policy::concurrent>(
        [&hits](int value) noexcept
        {
            hits += value;
        });
    concurrent_once(4);
    concurrent_once(7);
    CASTLE_SAMPLE_CHECK(hits == 9);

    policy::every_n every_third{3U, [&hits]() noexcept { ++hits; }};
    CASTLE_SAMPLE_CHECK(every_third.interval() == 3U);
    every_third();
    every_third();
    every_third();
    CASTLE_SAMPLE_CHECK(hits == 10);

    auto every_call = policy::make_every_n(0U, [&hits]() noexcept { ++hits; });
    CASTLE_SAMPLE_CHECK(every_call.interval() == 1U);
    every_call();
    CASTLE_SAMPLE_CHECK(hits == 11);

    auto compile_time_every = policy::make_every_n_ct<2U>([&hits]() noexcept { ++hits; });
    compile_time_every();
    compile_time_every();
    CASTLE_SAMPLE_CHECK(hits == 12);

    int changed_value = 0;
    int change_count = 0;
    auto change_policy = policy::make_on_change<int>(
        [&changed_value, &change_count](int value) noexcept
        {
            changed_value = value;
            ++change_count;
        });
    CASTLE_SAMPLE_CHECK(!change_policy.has_value());
    change_policy(8);
    change_policy(8);
    change_policy(9);
    CASTLE_SAMPLE_CHECK(change_count == 2);
    CASTLE_SAMPLE_CHECK(changed_value == 9);

    policy::on_change seeded_change{
        4,
        [&changed_value, &change_count](int value) noexcept
        {
            changed_value = value;
            ++change_count;
        }
    };
    CASTLE_SAMPLE_CHECK(seeded_change.has_value());
    seeded_change(4);
    seeded_change(5);
    CASTLE_SAMPLE_CHECK(change_count == 3);
    CASTLE_SAMPLE_CHECK(changed_value == 5);

    fake_clock::now_ms = 0;
    int timed_hits = 0;
    auto armed = policy::make_armed_window<policy::single_thread, fake_clock>(
        10_ms,
        [&timed_hits]() noexcept
        {
            ++timed_hits;
        });
    CASTLE_SAMPLE_CHECK(armed.armed());
    armed.execute();
    CASTLE_SAMPLE_CHECK(armed.fired());
    CASTLE_SAMPLE_CHECK(timed_hits == 1);
    armed.reset();
    fake_clock::now_ms = 11;
    armed.execute();
    CASTLE_SAMPLE_CHECK(armed.expired());
    CASTLE_SAMPLE_CHECK(timed_hits == 1);
    armed.rearm(5_ms);
    CASTLE_SAMPLE_CHECK(armed.window().count() == 5);
    CASTLE_SAMPLE_CHECK(armed.deadline().time_since_epoch().count() == 16);
    armed.expire();
    CASTLE_SAMPLE_CHECK(armed.expired());

    fake_clock::now_ms = 100;
    auto throttle = policy::make_throttle<policy::single_thread, fake_clock>(
        10_ms,
        [&timed_hits]() noexcept
        {
            ++timed_hits;
        });
    throttle();
    throttle();
    CASTLE_SAMPLE_CHECK(timed_hits == 2);
    fake_clock::now_ms = 111;
    throttle.execute();
    CASTLE_SAMPLE_CHECK(timed_hits == 3);
    throttle.reset();
    throttle();
    CASTLE_SAMPLE_CHECK(timed_hits == 4);

    fake_clock::now_ms = 200;
    int periodic_hits = 0;
    auto periodic = policy::make_periodic<policy::single_thread, fake_clock>(
        10_ms,
        [&periodic_hits]() noexcept
        {
            ++periodic_hits;
        });
    periodic.poll();
    CASTLE_SAMPLE_CHECK(periodic_hits == 0);
    fake_clock::now_ms = 225;
    periodic.poll();
    CASTLE_SAMPLE_CHECK(periodic_hits == 2);
    CASTLE_SAMPLE_CHECK(periodic.period().count() == 10);
    CASTLE_SAMPLE_CHECK(periodic.next_deadline().time_since_epoch().count() == 230);
    periodic.reset();
    CASTLE_SAMPLE_CHECK(periodic.next_deadline().time_since_epoch().count() == 235);

    return 0;
}
