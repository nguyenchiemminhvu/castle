// -----------------------------------------------------------------------------
// sample_tick_timer.cpp
//
// Beginner-friendly demo for castle::events::tick_timer<>.
//
// Big picture:
//
//    +----------------------+       calls on_tick(1)       +----------------+
//    |  tick_interrupt      |  ------------------------>   |  tick_timer<>  |
//    |  (a std::thread that |    every 100 ms              |                |
//    |   sleeps 100 ms)     |                              |  fires the     |
//    +----------------------+                              |  callback when |
//                                                          |  counter >=    |
//                                                          |  period        |
//                                                          +----------------+
//                                                                  |
//                                                                  v
//                                                          your registered
//                                                          callback runs
//
// The demo runs three phases:
//   1) one_shot  - callback fires exactly ONCE.
//   2) n_repeat  - callback fires exactly N times.
//   3) periodic  - callback fires forever until you press Ctrl+C.
// -----------------------------------------------------------------------------

#include "castle/callbacks/function.h"
#include "castle/callbacks/callback_registry.h"
#include "castle/events/tick_timer.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <mutex>
#include <thread>

using namespace castle::events;
using namespace castle::callbacks;
using namespace std::chrono_literals;

// -----------------------------------------------------------------------------
// One physical "tick" is 100 ms of wall-clock time. Small enough to see the
// demo run quickly; large enough to read the output line by line.
// -----------------------------------------------------------------------------
constexpr auto kTickPeriod = 100ms;

// -----------------------------------------------------------------------------
// The timer we drive from the "interrupt" thread. Callback signature is
// void() by design of tick_timer<>. Capacity = 1 subscriber (this demo only
// registers one at a time).
// -----------------------------------------------------------------------------
using tick_timer_t = tick_timer<1>;

// A single shared mutex protects the timer from concurrent access between
// the interrupt thread and the main thread.
tick_timer_t    g_timer;
std::mutex g_timer_mtx;

// Ctrl+C flag - set from the signal handler.
std::atomic<bool> g_stop_requested{false};

void on_sigint(int)
{
    g_stop_requested.store(true);
}

// -----------------------------------------------------------------------------
// The "timer interrupt" thread.
//
// On a real MCU this would be a SysTick ISR firing every N milliseconds.
// On Linux we just sleep for kTickPeriod and then call on_tick(1).
// -----------------------------------------------------------------------------
void tick_interrupt_thread(std::atomic<bool>& running)
{
    while (running.load())
    {
        std::this_thread::sleep_for(kTickPeriod);

        // Protect the timer against races with the main thread that
        // starts / stops / registers callbacks.
        std::lock_guard<std::mutex> lock(g_timer_mtx);
        g_timer.on_tick(1);
    }
}

// -----------------------------------------------------------------------------
// Our timeout callback (a plain free function). function_ct<> binds it at
// compile time with zero storage overhead.
// -----------------------------------------------------------------------------
int g_fire_count = 0;

void on_timeout()
{
    ++g_fire_count;
    std::cout << "  [callback] fire #" << g_fire_count << "\n";
}

int main()
{
    std::signal(SIGINT, on_sigint);

    // Start the "interrupt" thread.
    std::atomic<bool> interrupt_running{true};
    std::thread interrupt(tick_interrupt_thread, std::ref(interrupt_running));

    // Register our callback once - it will be reused across all 3 phases.
    function_ct<&on_timeout> cb;   // zero-storage compile-time binding
    {
        std::lock_guard<std::mutex> lock(g_timer_mtx);
        g_timer.register_callback(&cb);
        g_timer.set_period(10);     // fire every 10 ticks = 1 second
    }

    // -------------------------------------------------------------------------
    // Phase 1: one_shot - fires exactly once.
    // -------------------------------------------------------------------------
    std::cout << "\n=== Phase 1: one_shot (fires 1 time) ===\n";
    g_fire_count = 0;
    {
        std::lock_guard<std::mutex> lock(g_timer_mtx);
        g_timer.start(tick_timer_mode::one_shot);
    }
    // Wait long enough for the single fire to happen.
    std::this_thread::sleep_for(2s);
    std::cout << "  total fires = " << g_fire_count << "\n";

    // -------------------------------------------------------------------------
    // Phase 2: n_repeat - fires exactly N times, then stops on its own.
    // -------------------------------------------------------------------------
    constexpr std::uint32_t kRepeatCount = 3;
    std::cout << "\n=== Phase 2: n_repeat (fires " << kRepeatCount << " times) ===\n";
    g_fire_count = 0;
    {
        std::lock_guard<std::mutex> lock(g_timer_mtx);
        g_timer.start(tick_timer_mode::n_repeat, kRepeatCount);
    }
    // Wait long enough for all N fires to happen.
    std::this_thread::sleep_for(kRepeatCount * 1s + 500ms);
    std::cout << "  total fires = " << g_fire_count << "\n";

    // -------------------------------------------------------------------------
    // Phase 3: periodic - fires forever until Ctrl+C.
    // -------------------------------------------------------------------------
    std::cout << "\n=== Phase 3: periodic (press Ctrl+C to stop) ===\n";
    g_fire_count = 0;
    {
        std::lock_guard<std::mutex> lock(g_timer_mtx);
        g_timer.start(tick_timer_mode::periodic);
    }
    while (!g_stop_requested.load())
    {
        std::this_thread::sleep_for(100ms);
    }
    {
        std::lock_guard<std::mutex> lock(g_timer_mtx);
        g_timer.stop();
    }
    std::cout << "\n  Ctrl+C received. total fires = " << g_fire_count << "\n";

    // Shut down the interrupt thread cleanly.
    interrupt_running.store(false);
    interrupt.join();

    std::cout << "Bye.\n";
    return 0;
}
