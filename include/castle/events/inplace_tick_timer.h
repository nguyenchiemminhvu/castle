#pragma once

#include "castle/callbacks/inplace_function.h"
#include "castle/callbacks/inplace_callback_registry.h"
#include "castle/events/event_subscription.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

using castle::callbacks::inplace_function;
using castle::callbacks::inplace_callback_registry;
using castle::callbacks::callback_subscription_error;
using castle::callbacks::callback_subscription;

namespace castle
{
namespace events
{

// -----------------------------------------------------------------------------
// Repeat mode for inplace_tick_timer.
// -----------------------------------------------------------------------------
enum class inplace_tick_timer_mode : std::uint8_t
{
    one_shot = 0,   // fire once, then stop
    periodic,       // fire forever until stop()
    n_repeat        // fire N times, then stop
};

// -----------------------------------------------------------------------------
// inplace_tick_timer - fixed-capacity, heap-free, tick-driven software timer
// that OWNS its callbacks by value (via inplace_callback_registry).
//
// Model:
//   - Callback signature is void() by design: a timeout event carries no
//     payload; state must live in the captured closure / bound object.
//   - The timer is a passive accumulator. It does not read any clock and
//     does not own a thread. The tick source (ISR / RTOS task / event loop)
//     drives it via on_tick(elapsed_ticks).
//
// Template parameters:
//   MaxCallback              - max concurrent subscribers.
//   CallbackStorageSize      - inplace_function SBO size per callback.
//   CallbackStorageAlignment - inplace_function SBO alignment per callback.
//
// Usage:
//   castle::events::inplace_tick_timer<4> t;
//   t.set_period(100);                                       // 100 ticks
//   auto sub = t.register_callback([]{ /* on timeout */ });
//   t.start(castle::events::inplace_tick_timer_mode::periodic);
//
//   // From SysTick / RTOS tick / event loop:
//   t.on_tick(1);                                            // advance 1 tick
//
//   // Later:
//   sub.unsubscribe();
//   t.stop();
// -----------------------------------------------------------------------------
template <
    std::size_t MaxCallback,
    std::size_t CallbackStorageSize = 32,
    std::size_t CallbackStorageAlignment = alignof(std::max_align_t)>
class inplace_tick_timer
{
    static_assert(MaxCallback > 0,
                  "inplace_tick_timer requires MaxCallback >= 1");

public:
    using tick_type = std::uint32_t;
    using error = event_subscription_error;
    using mode = inplace_tick_timer_mode;
    using subscription = callback_subscription;
    using registry_type = inplace_callback_registry<
        MaxCallback,
        void(),
        CallbackStorageSize,
        CallbackStorageAlignment>;

    inplace_tick_timer() = default;
    ~inplace_tick_timer() = default;

    // Non-copyable, non-movable. Registry embeds an unsubscribable identity
    // that outstanding subscriptions reference by address.
    inplace_tick_timer(const inplace_tick_timer&) = delete;
    inplace_tick_timer& operator=(const inplace_tick_timer&) = delete;

    inplace_tick_timer(inplace_tick_timer&&) = delete;
    inplace_tick_timer& operator=(inplace_tick_timer&&) = delete;

    // -------------------------------------------------------------------------
    // Configure the timer period in ticks. Safe to call while running: takes
    // effect from the next full cycle. Does NOT rearm a stopped timer.
    // -------------------------------------------------------------------------
    error set_period(tick_type period_ticks) noexcept
    {
        if (period_ticks == 0)
        {
            return error::invalid_config;
        }
        period_ = period_ticks;
        return error::ok;
    }

    // -------------------------------------------------------------------------
    // Register a timeout callback. Any callable convertible to inplace_function
    // signature void() is accepted (function pointers, stateless lambdas,
    // stateful lambdas within the SBO budget, functor objects, etc.).
    //
    // Returns an callback_subscription. On failure the handle is
    // !valid() and out_error (if provided) is set.
    // -------------------------------------------------------------------------
    template <typename Callback>
    subscription register_callback(Callback&& callback, error* out_error = nullptr) noexcept
    {
        callback_subscription_error inner_error = callback_subscription_error::ok;

        subscription sub = registry_.subscribe(std::forward<Callback>(callback), &inner_error);

        if (out_error != nullptr)
        {
            *out_error = convert_error(inner_error);
        }

        return sub;
    }

    // -------------------------------------------------------------------------
    // Start (or restart) the timer.
    //
    //   run_mode      - one_shot / periodic / n_repeat
    //   repeat_count  - only used when run_mode == n_repeat; must be > 0.
    //                   Ignored otherwise.
    //
    // Any previously accumulated tick counter is cleared. Callbacks are NOT
    // cleared (call clear_callbacks() if that is desired).
    // -------------------------------------------------------------------------
    error start(mode run_mode = mode::periodic, tick_type repeat_count = 0) noexcept
    {
        if (period_ == 0)
        {
            return error::not_configured;
        }

        if (run_mode == mode::n_repeat && repeat_count == 0)
        {
            return error::invalid_config;
        }

        mode_ = run_mode;
        repeat_remaining_ = (run_mode == mode::n_repeat)
                            ? repeat_count
                            : static_cast<tick_type>(0);
        counter_ = 0;
        running_ = true;
        return error::ok;
    }

    // -------------------------------------------------------------------------
    // Stop the timer AND reset the tick counter. After stop() the timer is
    // idle: subsequent on_tick() calls are no-ops until start() is called
    // again. Registered callbacks are preserved.
    // -------------------------------------------------------------------------
    void stop() noexcept
    {
        running_ = false;
        counter_ = 0;
        repeat_remaining_ = 0;
    }

    // -------------------------------------------------------------------------
    // Pause the timer WITHOUT resetting the tick counter. resume() picks up
    // exactly where pause() left off. Useful for e.g. debounce logic that
    // wants to freeze on external state changes.
    // -------------------------------------------------------------------------
    void pause() noexcept
    {
        running_ = false;
    }

    // -------------------------------------------------------------------------
    // Resume a paused timer. No effect if the timer was never configured,
    // or if it is currently running.
    // -------------------------------------------------------------------------
    error resume() noexcept
    {
        if (period_ == 0)
        {
            return error::not_configured;
        }
        running_ = true;
        return error::ok;
    }

    // -------------------------------------------------------------------------
    // Reset the tick counter to zero without changing running / mode state.
    // Effectively "start the current cycle over".
    // -------------------------------------------------------------------------
    void reset() noexcept
    {
        counter_ = 0;
    }

    // -------------------------------------------------------------------------
    // Advance the timer by elapsed_ticks. Invokes registered callbacks each
    // time counter_ crosses period_.
    //
    // If elapsed_ticks spans multiple periods (e.g. a slow tick source or a
    // long ISR-disabled window), the timer fires MULTIPLE times in one call
    // for a periodic timer, or catches up correctly for n_repeat / one_shot.
    // This is the standard "catch-up" behaviour for software timers and keeps
    // long-term timing drift-free.
    //
    // Safe to call from an ISR / RTOS tick hook: no allocation, no locking.
    // Callbacks execute in the caller's context — keep them short.
    // -------------------------------------------------------------------------
    void on_tick(tick_type elapsed_ticks = 1) noexcept
    {
        if (!running_ || period_ == 0)
        {
            return;
        }

        counter_ += elapsed_ticks;

        while (running_ && counter_ >= period_)
        {
            counter_ -= period_;

            // Fire the timeout event. Callbacks run in registration order.
            registry_.invoke();

            // Post-fire mode handling.
            switch (mode_)
            {
                case mode::one_shot:
                {
                    running_ = false;
                    counter_ = 0;
                    break;
                }
                case mode::n_repeat:
                {
                    if (repeat_remaining_ > 0)
                    {
                        --repeat_remaining_;
                    }
                    if (repeat_remaining_ == 0)
                    {
                        running_ = false;
                        counter_ = 0;
                    }
                    break;
                }
                case mode::periodic:
                default:
                {
                    // Keep running; loop consumes any remaining accumulated ticks.
                    break;
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Introspection / queries.
    // -------------------------------------------------------------------------
    bool is_running() const noexcept
    {
        return running_;
    }

    tick_type period() const noexcept
    {
        return period_;
    }

    tick_type elapsed() const noexcept
    {
        return counter_;
    }

    // Remaining ticks until the next timeout. Returns 0 when the timer is
    // stopped or not configured (no meaningful remaining time).
    tick_type remaining() const noexcept
    {
        if (!running_ || period_ == 0)
        {
            return 0;
        }
        return (counter_ >= period_)
                ? static_cast<tick_type>(0)
                : static_cast<tick_type>(period_ - counter_);
    }

    mode current_mode() const noexcept
    {
        return mode_;
    }

    // Number of remaining fires for an n_repeat timer. Undefined semantics
    // for other modes -> returns 0.
    tick_type repeats_remaining() const noexcept
    {
        return (mode_ == mode::n_repeat)
                ? repeat_remaining_
                : 0;
    }

    std::size_t callback_count() const noexcept
    {
        return registry_.size();
    }

    static constexpr std::size_t callback_capacity() noexcept
    {
        return MaxCallback;
    }

    static constexpr tick_type max_period() noexcept
    {
        return std::numeric_limits<tick_type>::max();
    }

    // -------------------------------------------------------------------------
    // Remove all registered callbacks. Outstanding subscription handles
    // become stale. Timer running state / period / counter are untouched.
    // -------------------------------------------------------------------------
    void clear_callbacks() noexcept
    {
        registry_.clear();
    }

    // -------------------------------------------------------------------------
    // Direct access to the underlying registry (advanced use — e.g. to plug
    // this timer into a generic subscription manager). Prefer
    // register_callback() for normal use.
    // -------------------------------------------------------------------------
    registry_type& registry() noexcept
    {
        return registry_;
    }

    const registry_type& registry() const noexcept
    {
        return registry_;
    }

private:
    // Map callback_subscription_error to tick_timer_error.
    static constexpr error convert_error(callback_subscription_error error_code) noexcept
    {
        switch (error_code)
        {
            case callback_subscription_error::ok:
            {
                return error::ok;
            }
            case callback_subscription_error::full:
            {
                return error::full;
            }
            case callback_subscription_error::invalid_callback:
            {
                return error::invalid_callback;
            }
            case callback_subscription_error::invalid_subscription:
            {
                return error::invalid_subscription;
            }
        }

        return error::invalid_subscription;
    }

private:
    registry_type registry_{};

    // Timer state. All ordinary integers — no atomics: this class is designed
    // to be driven from a single tick source. If the tick source is an ISR
    // and subscribe/start are called from thread context, wrap those calls
    // with an appropriate critical section externally.
    tick_type period_           = 0;   // 0 => not configured
    tick_type counter_          = 0;   // accumulated ticks in current cycle
    tick_type repeat_remaining_ = 0;   // remaining fires for n_repeat mode
    mode      mode_             = mode::periodic;
    bool      running_          = false;
};

} // namespace events
} // namespace castle
