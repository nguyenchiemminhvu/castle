// -----------------------------------------------------------------------------
// sample_inplace_tick_timer.cpp
//
// Beginner-friendly demo for castle::events::inplace_tick_timer<>.
//
// Big picture:
//
//    +----------------------+       calls on_tick(1)       +----------------------+
//    |  tick_interrupt      |  ------------------------>   | inplace_tick_timer<> |
//    |  (a std::thread that |    every 100 ms              |                      |
//    |   sleeps 100 ms)     |                              | owns the callback    |
//    +----------------------+                              | and fires it when    |
//                                                          | counter >= period   |
//                                                          +----------------------+
//                                                                    |
//                                                                    v
//                                                           registered lambda runs
//
// The demo runs three phases:
//   1) one_shot  - callback fires exactly ONCE.
//   2) n_repeat  - callback fires exactly N times.
//   3) periodic  - callback fires continuously until Ctrl+C.
//
// Unlike the old tick_timer demo, inplace_tick_timer<> OWNS the callback.
// Therefore we can register a stateful lambda directly:
//
//     auto subscription = timer.register_callback(
//         [&fire_count]() {
//             ++fire_count;
//         });
//
// The lambda is stored inside the timer's fixed-size callback storage.
// No dynamic allocation is required by the timer/callback registry.
// -----------------------------------------------------------------------------

#include "castle/events/inplace_tick_timer.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>

using namespace castle::events;
using namespace std::chrono_literals;

// -----------------------------------------------------------------------------
// One physical "tick" is 100 ms of wall-clock time.
//
// In a real MCU this could correspond to a SysTick / timer ISR.
// -----------------------------------------------------------------------------
constexpr auto kTickPeriod = 100ms;

// -----------------------------------------------------------------------------
// Timer configuration.
//
// MaxCallback = 1
//     Only one callback is registered in this demo.
//
// CallbackStorageSize = 32
//     The callback registry stores the callback inside an inplace_function.
//     The lambda below must fit into this storage.
//
// The timer does not allocate memory and does not own a thread.
// -----------------------------------------------------------------------------
using tick_timer_t = inplace_tick_timer<
    1,      // MaxCallback
    32      // CallbackStorageSize
>;

tick_timer_t g_timer;
std::mutex g_timer_mtx;

// Ctrl+C flag.
std::atomic<bool> g_stop_requested{false};

// -----------------------------------------------------------------------------
// Ctrl+C signal handler.
//
// Keep signal handlers extremely small. Just request shutdown.
//
// std::atomic<bool> is used because the flag is shared with the main thread.
// -----------------------------------------------------------------------------
void on_sigint(int)
{
    g_stop_requested.store(true);
}

// -----------------------------------------------------------------------------
// The "timer interrupt" thread.
//
// On a real MCU this would be something like:
//
//     SysTick_Handler()
//     {
//         timer.on_tick(1);
//     }
//
// Here Linux gives us a normal thread that sleeps for 100 ms.
//
// IMPORTANT:
// inplace_tick_timer itself does NOT create this thread.
// It only accumulates ticks supplied by its caller.
// -----------------------------------------------------------------------------
void tick_interrupt_thread(std::atomic<bool>& running)
{
    while (running.load())
    {
        std::this_thread::sleep_for(kTickPeriod);

        // Protect the timer because the main thread also calls
        // start(), stop(), etc.
        //
        // In a real ISR-based system you would normally use a
        // critical section / interrupt protection mechanism instead.
        std::lock_guard<std::mutex> lock(g_timer_mtx);

        g_timer.on_tick(1);
    }
}

int main()
{
    std::signal(SIGINT, on_sigint);

    // -------------------------------------------------------------------------
    // Start the simulated hardware tick source.
    // -------------------------------------------------------------------------
    std::atomic<bool> interrupt_running{true};

    std::thread interrupt(
        tick_interrupt_thread,
        std::ref(interrupt_running)
    );

    // -------------------------------------------------------------------------
    // Register the callback.
    //
    // IMPORTANT DIFFERENCE from the old tick_timer:
    //
    //     old:
    //         function_ct<&on_timeout> cb;
    //         timer.register_callback(&cb);
    //
    //     inplace:
    //         timer.register_callback(lambda);
    //
    // inplace_tick_timer owns the callback through inplace_callback_registry.
    // Therefore the lambda can safely contain state.
    //
    // fire_count is atomic because the callback executes on the interrupt
    // thread while main() reads it.
    // -------------------------------------------------------------------------
    std::atomic<std::uint32_t> fire_count{0};

    auto subscription = g_timer.register_callback(
        [&fire_count]()
        {
            const std::uint32_t count =
                fire_count.fetch_add(1) + 1;

            std::cout
                << "  [callback] fire #"
                << count
                << "\n";
        }
    );

    if (!subscription.valid())
    {
        std::cerr << "ERROR: failed to register callback\n";

        interrupt_running.store(false);
        interrupt.join();

        return 1;
    }

    // -------------------------------------------------------------------------
    // Configure:
    //
    // 10 ticks × 100 ms = 1 second.
    // -------------------------------------------------------------------------
    {
        std::lock_guard<std::mutex> lock(g_timer_mtx);

        const auto result = g_timer.set_period(10);

        if (result != tick_timer_t::error::ok)
        {
            std::cerr << "ERROR: set_period() failed\n";

            interrupt_running.store(false);
            interrupt.join();

            return 1;
        }
    }

    // -------------------------------------------------------------------------
    // Phase 1: one_shot
    //
    // Expected:
    //
    //     [callback] fire #1
    //     total fires = 1
    //
    // After the callback executes, the timer automatically stops.
    // -------------------------------------------------------------------------
    std::cout
        << "\n=== Phase 1: one_shot (fires 1 time) ===\n";

    fire_count.store(0);

    {
        std::lock_guard<std::mutex> lock(g_timer_mtx);

        const auto result =
            g_timer.start(inplace_tick_timer_mode::one_shot);

        if (result != tick_timer_t::error::ok)
        {
            std::cerr << "ERROR: one_shot start() failed\n";
        }
    }

    // Wait long enough for the one-second timeout.
    std::this_thread::sleep_for(2s);

    std::cout
        << "  total fires = "
        << fire_count.load()
        << "\n";

    // -------------------------------------------------------------------------
    // Phase 2: n_repeat
    //
    // Expected:
    //
    //     [callback] fire #1
    //     [callback] fire #2
    //     [callback] fire #3
    //     total fires = 3
    //
    // After the third callback the timer automatically stops.
    // -------------------------------------------------------------------------
    constexpr std::uint32_t kRepeatCount = 3;

    std::cout
        << "\n=== Phase 2: n_repeat (fires "
        << kRepeatCount
        << " times) ===\n";

    fire_count.store(0);

    {
        std::lock_guard<std::mutex> lock(g_timer_mtx);

        const auto result =
            g_timer.start(
                inplace_tick_timer_mode::n_repeat,
                kRepeatCount
            );

        if (result != tick_timer_t::error::ok)
        {
            std::cerr << "ERROR: n_repeat start() failed\n";
        }
    }

    // 3 × 1 second + some margin.
    std::this_thread::sleep_for(
        kRepeatCount * 1s + 500ms
    );

    std::cout
        << "  total fires = "
        << fire_count.load()
        << "\n";

    // -------------------------------------------------------------------------
    // Phase 3: periodic
    //
    // Expected:
    //
    //     [callback] fire #1
    //     [callback] fire #2
    //     [callback] fire #3
    //     ...
    //
    // The timer continues until Ctrl+C.
    // -------------------------------------------------------------------------
    std::cout
        << "\n=== Phase 3: periodic (press Ctrl+C to stop) ===\n";

    fire_count.store(0);

    {
        std::lock_guard<std::mutex> lock(g_timer_mtx);

        const auto result =
            g_timer.start(inplace_tick_timer_mode::periodic);

        if (result != tick_timer_t::error::ok)
        {
            std::cerr << "ERROR: periodic start() failed\n";
        }
    }

    while (!g_stop_requested.load())
    {
        std::this_thread::sleep_for(100ms);
    }

    // Stop the timer after Ctrl+C.
    {
        std::lock_guard<std::mutex> lock(g_timer_mtx);

        g_timer.stop();
    }

    std::cout
        << "\n  Ctrl+C received. total fires = "
        << fire_count.load()
        << "\n";

    // -------------------------------------------------------------------------
    // Demonstrate subscription lifetime.
    //
    // The callback can be explicitly removed through the subscription handle.
    //
    // After unsubscribe(), the timer remains alive but has no callback.
    // -------------------------------------------------------------------------
    const auto unsubscribe_result = subscription.unsubscribe();

    if (unsubscribe_result ==
        castle::callbacks::callback_subscription_error::ok)
    {
        std::cout << "  callback unsubscribed successfully.\n";
    }

    // -------------------------------------------------------------------------
    // Shut down the simulated interrupt thread cleanly.
    // -------------------------------------------------------------------------
    interrupt_running.store(false);
    interrupt.join();

    std::cout << "Bye.\n";

    return 0;
}
