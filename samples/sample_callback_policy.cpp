#include "castle/callbacks/callback_policy.h"
#include "castle/callbacks/function.h"
#include "castle/callbacks/inplace_function.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace castle::callbacks;
using namespace std::chrono_literals;

//=====================================================
// Helpers used across the demos
//=====================================================
void free_function(int value)
{
    std::cout << "  free_function     : " << value << std::endl;
}

struct Handler
{
    void on_tick(int value)
    {
        std::cout << "  Handler::on_tick  : " << value << std::endl;
    }
};

static void section(const char* title)
{
    std::cout << "\n==============================\n"
              << title
              << "\n==============================" << std::endl;
}

int main()
{
    //-------------------------------------------------------------------------
    // once - fire exactly once during the object's lifetime
    //-------------------------------------------------------------------------
    section("once::single_thread (raw lambda)");
    {
        auto init = policy::once::make_policy_st(
            []() {
                std::cout << "  boot_hw() ran once" << std::endl;
            }
        );

        init();                       // fires
        init();                       // no-op
        init.reset();
        init();                       // fires again after reset
        init();                       // fires again after reset

        init.reset();
        // special: because the policy is a callable type, it can be wrapped in a function_f
        auto init_f_with_policy = make_function_f<void()>(init);
        init_f_with_policy();          // fires again after reset
        init_f_with_policy();          // no-op

        init.reset();
        // special: because the policy is a callable type, it can be wrapped in a function_fr
        auto init_fr_with_policy = make_function_fr<void()>(init);
        init_fr_with_policy();          // fires again after reset
        init_fr_with_policy();          // no-op

        init.reset();
        // special: because the policy is a callable type, it can be wrapped in an inplace_function
        inplace_function<void()> init_erased{ std::move(init) };
        init_erased();                 // fires again after reset
        init_erased();                 // no-op
    }

    section("once::single_thread (compile-time bound free function)");
    {
        // Wrap a compile-time free function callback inside a `once` policy.
        function_ct<&free_function> cb;
        // auto once_free = policy::once::single_thread<decltype(cb)>(cb);
        auto once_free = policy::once::make_policy_st(cb); // better declaration syntax
        once_free(42);
        once_free(43);                // suppressed
    }

    section("once::concurrent (atomic CAS)");
    {
        auto log_once = policy::once::make_policy_concurrent(
            []{ std::cout << "  log_once fired" << std::endl; }
        );

        // Simulate two threads racing on the same policy.
        std::thread t1([&]{ log_once(); });
        std::thread t2([&]{ log_once(); });
        t1.join();
        t2.join();
        std::cout << "  has_fired = " << log_once.has_fired() << std::endl;
    }

    //-------------------------------------------------------------------------
    // armed_window - at most one fire, only inside a validity window
    //-------------------------------------------------------------------------
    section("armed_window::single_thread (fires inside window)");
    {
        auto ack = policy::armed_window::make_policy_st(
            200ms,
            []{ std::cout << "  send_ack() inside window" << std::endl; }
        );

        ack();                        // fires (armed -> fired)
        ack();                        // no-op (already fired)
        std::cout << "  fired = " << ack.fired() << std::endl;
    }

    section("armed_window::single_thread (expires without firing)");
    {
        auto ack = policy::armed_window::make_policy_st(
            20ms,
            []{ std::cout << "  send_ack() (should NOT print)" << std::endl; }
        );

        std::this_thread::sleep_for(40ms);
        ack();                        // expired, no fire
        std::cout << "  expired = " << ack.expired() << std::endl;

        ack.rearm(50ms);              // re-open window
        ack();                        // fires now
        std::cout << "  fired after rearm = " << ack.fired() << std::endl;
    }

    //-------------------------------------------------------------------------
    // every_n - fire once every N invocations
    //-------------------------------------------------------------------------
    section("every_n::single_thread (runtime N)");
    {
        auto hb = policy::every_n::make_policy_st(
            3,
            []{ std::cout << "  toggle_led()" << std::endl; }
        );

        for (int i = 1; i <= 7; ++i)
        {
            std::cout << " tick " << i << ":";
            hb();                     // fires on 3, 6
        }
    }

    section("every_n::make_policy_ct<N> (compile-time N)");
    {
        auto stats = policy::every_n::make_policy_ct<4>(
            []{ std::cout << "  dump_stats()" << std::endl; }
        );

        for (int i = 1; i <= 8; ++i)
        {
            std::cout << " sample " << i << ":";
            stats();                  // fires on 4, 8
        }
    }

    //-------------------------------------------------------------------------
    // on_change - fire only when observed value differs from previous
    //-------------------------------------------------------------------------
    section("on_change::single_thread (with initial value)");
    {
        auto watcher = policy::on_change::make_policy_st(
            0,
            [](int v){ std::cout << "  changed -> " << v << std::endl; }
        );

        watcher(0);                   // unchanged
        watcher(1);                   // fires
        watcher(1);                   // unchanged
        watcher(2);                   // fires
    }

    section("on_change::single_thread<T> (no initial value)");
    {
        auto watcher = policy::on_change::make_policy_st<int>(
            [](int v){ std::cout << "  first-or-changed -> " << v << std::endl; }
        );

        watcher(7);                   // fires (first sample)
        watcher(7);                   // unchanged
        watcher(8);                   // fires
    }

    //-------------------------------------------------------------------------
    // throttle - drop calls that come sooner than `interval`
    //-------------------------------------------------------------------------
    section("throttle::single_thread (50ms interval)");
    {
        auto gate = policy::throttle::make_policy_st(
            50ms,
            [](const char* m){ std::cout << "  log: " << m << std::endl; }
        );

        gate("first");                // fires (no last time yet)
        gate("suppressed");           // dropped
        std::this_thread::sleep_for(60ms);
        gate("after interval");       // fires
    }

    //-------------------------------------------------------------------------
    // periodic - poll-driven, with catch-up
    //-------------------------------------------------------------------------
    section("periodic::single_thread (30ms period, with catch-up)");
    {
        Handler h;
        // Bind a runtime member-function callback and drive it via periodic.
        function_m<Handler, void(int)> mcb(h, &Handler::on_tick);

        int tick = 0;
        auto tock = policy::periodic::make_policy_st(
            30ms,
            [&]{ mcb(++tick); }
        );

        // First poll: nothing yet (deadline not reached).
        tock();
        std::this_thread::sleep_for(100ms);
        // ~3 periods elapsed -> catch-up fires that many times.
        tock();

        std::cout << "  total ticks = " << tick << std::endl;
    }

    //-------------------------------------------------------------------------
    // inplace_function integration - stable stored callable type across
    // policies. Useful when the wrapping site needs a type-erased callback
    // (e.g. stored in a table) but must still avoid heap allocation.
    //-------------------------------------------------------------------------
    section("once + inplace_function (type-erased, SBO)");
    {
        using cb_t = inplace_function<void(int)>;

        cb_t erased{ &free_function };
        auto boot_once = policy::once::make_policy_st(std::move(erased));

        boot_once(101);   // fires
        boot_once(102);   // suppressed
        std::cout << "  has_fired = " << boot_once.has_fired() << std::endl;
    }

    section("every_n + inplace_function (member function via lambda)");
    {
        using cb_t = inplace_function<void(int)>;

        Handler h;
        cb_t erased{ [&h](int v){ h.on_tick(v); } };

        auto every3 = policy::every_n::make_policy_st(3, std::move(erased));
        for (int i = 1; i <= 7; ++i)
        {
            std::cout << " tick " << i << ":";
            every3(i);    // fires on i=3, i=6
        }
    }

    section("throttle + inplace_function (capturing lambda)");
    {
        using cb_t = inplace_function<void(const char*), 128>;

        int hits = 0;
        cb_t erased{
            [&hits](const char* msg)
            {
                ++hits;
                std::cout << "  emit[" << hits << "]: " << msg << std::endl;
            }
        };

        auto gate = policy::throttle::make_policy_st(50ms, std::move(erased));
        gate("first");                // fires
        gate("suppressed");           // dropped
        std::this_thread::sleep_for(60ms);
        gate("after interval");       // fires
        std::cout << "  total hits = " << hits << std::endl;
    }

    section("on_change + inplace_function (compile-time bound free func)");
    {
        // Wrap function_ct in an inplace_function so on_change sees a stable
        // stored type but still no heap allocation.
        function_ct<&free_function> ct_cb;
        inplace_function<void(int)> erased{ ct_cb };

        auto watcher = policy::on_change::make_policy_st(0, std::move(erased));
        watcher(0);                   // unchanged
        watcher(1);                   // fires
        watcher(1);                   // unchanged
        watcher(2);                   // fires
    }

    std::cout << "\nAll callback_policy demos done." << std::endl;
    return 0;
}