// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-capacity, tick-driven software timer with value-owned callbacks.
 *
 * Use this timer when a deterministic loop, ISR, or scheduler already supplies
 * elapsed ticks and you need timeout callbacks without creating threads or
 * allocating memory. Callback storage is fixed at compile time, callbacks are
 * invoked in subscription order, and all execution happens in the caller of
 * on_tick().
 *
 * @warning The class provides no internal synchronization. Drive ticking and
 * callback registration from one execution context, or protect mixed contexts
 * externally.
 *
 * @code
 * #include "castle/events/tick_timer.hpp"
 *
 * int main()
 * {
 *     castle::events::tick_timer<2U> timer;
 *     volatile std::uint32_t fired = 0U;
 *
 *     timer.set_period(5U);
 *     timer.register_callback([&fired]() { ++fired; });
 *     timer.start(castle::events::tick_timer_mode::periodic);
 *
 *     timer.on_tick(2U);
 *     timer.on_tick(3U);
 *
 *     return fired == 1U ? 0 : 1;
 * }
 * @endcode
 */
#ifndef CASTLE_EVENTS_TICK_TIMER_HPP
#define CASTLE_EVENTS_TICK_TIMER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/error/status.hpp"

#include "castle/callbacks/function.hpp"
#include "castle/callbacks/function_registry.hpp"

#include <stdint.h>

namespace castle
{
namespace events
{

/**
 * @brief Timer run modes supported by tick_timer.
 */
enum class tick_timer_mode : uint8_t
{
    one_shot = 0,   /**< @brief Fire once, then stop and clear accumulated ticks. */
    periodic,       /**< @brief Fire on every elapsed period until stop() or pause(). */
    n_repeat        /**< @brief Fire repeat_count times after start(mode::n_repeat, repeat_count). */
};

/**
 * @brief Tick-driven timer with fixed callback capacity and no internal heap use.
 *
 * @tparam MaxCallback Maximum number of simultaneously registered callbacks.
 * @tparam CallbackStorageSize Inline storage reserved for each callback object.
 * @tparam CallbackStorageAlignment Alignment of each callback's inline storage.
 *
 * @note The timer is passive: it never reads a clock and never creates a
 * thread. Progress occurs only when on_tick() is called.
 */
template <
    size_type MaxCallback,
    size_type CallbackStorageSize = castle::inplace_storage_reserved,
    size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class tick_timer
{
    static_assert(MaxCallback > 0,
                  "tick_timer requires MaxCallback >= 1");

public:
    /** @brief Tick counter type used for periods, elapsed time, and repeat counts. */
    using tick_type = uint32_t;

    /** @brief Error code type returned by configuration and control operations. */
    using error = castle::status;

    /** @brief Shorthand for tick_timer_mode. */
    using mode = tick_timer_mode;

    /** @brief Subscription handle returned by register_callback(). */
    using subscription = callbacks::subscription;

    /**
     * @brief Callback registry type embedded by the timer.
     *
     * @note This registry owns callbacks by value and preserves subscription
     * order during invocation.
     */
    using registry_type = callbacks::function_registry<
        MaxCallback,
        void(),
        CallbackStorageSize,
        CallbackStorageAlignment>;

    /** @brief Construct an unconfigured, stopped timer. */
    tick_timer() CASTLE_DEFAULT;

    /** @brief Destroy the timer. */
    ~tick_timer() CASTLE_DEFAULT;

    /** @brief Copy and move are disabled because subscriptions reference this timer's embedded registry. */
    tick_timer(CASTLE_CONST tick_timer&) CASTLE_DELETE;
    tick_timer& operator=(CASTLE_CONST tick_timer&) CASTLE_DELETE;

    tick_timer(tick_timer&&) CASTLE_DELETE;
    tick_timer& operator=(tick_timer&&) CASTLE_DELETE;

    /**
     * @brief Set the timer period in ticks.
     *
     * @param period_ticks Number of ticks required for one expiration.
     * @return error::ok when the period is non-zero, otherwise
     * error::invalid_config.
     *
     * @note Calling this while the timer is running updates the stored period
     * immediately but does not reset the accumulated counter.
     */
    error set_period(tick_type period_ticks) noexcept
    {
        if (period_ticks == 0)
        {
            return error::invalid_config;
        }
        period_ = period_ticks;
        return error::ok;
    }

    /**
     * @brief Register a timeout callback.
     *
     * @tparam Callback Callable type convertible to registry_type::callback_type.
     * @param callback Callable to store by value inside the timer registry.
     * @param out_error Optional output for the registration result.
     * @return A subscription handle. The returned handle is invalid when the
     * registry is full or the callback is invalid.
     *
     * @note Callback invocation order matches subscription order.
     * @warning Callback registration is not synchronized with on_tick().
     */
    template <typename Callback>
    subscription register_callback(Callback&& callback, error* out_error = nullptr) noexcept
    {
        status inner_error = status::ok;

        subscription sub = registry_.subscribe(CASTLE_FORWARD<Callback>(callback), &inner_error);

        if (out_error != nullptr)
        {
            *out_error = inner_error;
        }

        return sub;
    }

    /**
     * @brief Start or restart the timer.
     *
     * @param run_mode Expiration mode to use for subsequent ticks.
     * @param repeat_count Number of expirations for mode::n_repeat.
     * @return error::ok on success, error::not_configured when no non-zero
     * period has been set, or error::invalid_config when run_mode is
     * mode::n_repeat and repeat_count is zero.
     *
     * @note start() always clears the accumulated counter before running.
     */
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

    /**
     * @brief Stop the timer and clear accumulated progress.
     *
     * @note Registered callbacks remain installed.
     * @note stop() also clears repeat_remaining_, so resuming an n_repeat timer
     * after stop() does not restore the original repeat budget.
     */
    void stop() noexcept
    {
        running_ = false;
        counter_ = 0;
        repeat_remaining_ = 0;
    }

    /**
     * @brief Pause the timer without resetting the accumulated counter.
     *
     * @note pause() leaves the configured period, mode, and repeat state
     * unchanged.
     */
    void pause() noexcept
    {
        running_ = false;
    }

    /**
     * @brief Resume ticking with the current period and mode.
     *
     * @return error::ok when a non-zero period is configured, otherwise
     * error::not_configured.
     *
     * @note resume() simply sets the running flag. It does not restore a prior
     * repeat_count that was cleared by stop() or by a completed n_repeat run.
     */
    error resume() noexcept
    {
        if (period_ == 0)
        {
            return error::not_configured;
        }
        running_ = true;
        return error::ok;
    }

    /**
     * @brief Reset the accumulated ticks for the current cycle.
     *
     * @note The running state and mode are unchanged.
     */
    void reset() noexcept
    {
        counter_ = 0;
    }

    /**
     * @brief Advance the timer by a number of ticks.
     *
     * @param elapsed_ticks Number of ticks to add to the current cycle.
     *
     * @note When elapsed_ticks crosses multiple periods, periodic timers invoke
     * callbacks multiple times in one call to catch up.
     * @warning Callbacks run synchronously in the caller of on_tick().
     */
    void on_tick(tick_type elapsed_ticks = 1) noexcept
    {
        if (!running_ || period_ == 0) // LCOV_EXCL_BR_LINE
        {
            return;
        }

        counter_ += elapsed_ticks;

        while (running_ && counter_ >= period_)
        {
            counter_ -= period_;

            // Invoke in registry subscription order once per completed period.
            registry_.invoke();

            switch (mode_)
            {
                case mode::one_shot:
                {
                    // One-shot expiry consumes the event and returns to idle.
                    running_ = false;
                    counter_ = 0;
                    break;
                }
                case mode::n_repeat:
                {
                    if (repeat_remaining_ > 0) // LCOV_EXCL_BR_LINE
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
                {
                    CASTLE_FALL_THROUGH;
                }
                default:
                {
                    // Periodic mode stays armed so additional accumulated
                    // whole periods in this call keep firing.
                    break;
                }
            }
        }
    }

    /**
     * @brief Report whether the timer is currently accepting ticks.
     *
     * @return true when running, otherwise false.
     */
    bool is_running() CASTLE_CONST noexcept
    {
        return running_;
    }

    /**
     * @brief Read the configured period.
     *
     * @return The configured period in ticks, or zero when not configured.
     */
    tick_type period() CASTLE_CONST noexcept
    {
        return period_;
    }

    /**
     * @brief Read the ticks accumulated in the current cycle.
     *
     * @return The current tick counter.
     */
    tick_type elapsed() CASTLE_CONST noexcept
    {
        return counter_;
    }

    /**
     * @brief Report the ticks remaining until the next expiration.
     *
     * @return Zero when the timer is stopped or unconfigured, otherwise
     * period() - elapsed().
     */
    tick_type remaining() CASTLE_CONST noexcept
    {
        if (!running_ || period_ == 0)
        {
            return 0;
        }
        return (counter_ >= period_)
                ? static_cast<tick_type>(0)
                : static_cast<tick_type>(period_ - counter_);
    }

    /**
     * @brief Read the currently stored run mode.
     *
     * @return The mode most recently passed to start(), or the default periodic
     * mode before the first start().
     */
    mode current_mode() CASTLE_CONST noexcept
    {
        return mode_;
    }

    /**
     * @brief Read the remaining repeat budget for mode::n_repeat.
     *
     * @return The remaining repeat count when current_mode() is mode::n_repeat,
     * otherwise zero.
     */
    tick_type repeats_remaining() CASTLE_CONST noexcept
    {
        return (mode_ == mode::n_repeat)
                ? repeat_remaining_
                : 0;
    }

    /**
     * @brief Count active timeout callbacks.
     *
     * @return Number of currently subscribed callbacks.
     */
    size_type callback_count() CASTLE_CONST noexcept
    {
        return registry_.size();
    }

    /**
     * @brief Query the compile-time callback capacity.
     *
     * @return MaxCallback.
     */
    static CASTLE_CONSTEXPR size_type callback_capacity() noexcept
    {
        return MaxCallback;
    }

    /**
     * @brief Query the largest representable period value.
     *
     * @return The maximum value of tick_type.
     */
    static CASTLE_CONSTEXPR tick_type max_period() noexcept
    {
        return castle::numeric_limits<tick_type>::max();
    }

    /**
     * @brief Remove every registered callback.
     *
     * @note Running state, mode, period, and accumulated ticks are unchanged.
     * Outstanding subscription handles become stale.
     */
    void clear_callbacks() noexcept
    {
        registry_.clear();
    }

    /**
     * @brief Access the underlying callback registry.
     *
     * @return Mutable reference to the embedded registry.
     *
     * @warning Direct registry mutation is not synchronized with on_tick().
     */
    registry_type& registry() noexcept
    {
        return registry_;
    }

    /**
     * @brief Access the underlying callback registry.
     *
     * @return Const reference to the embedded registry.
     */
    CASTLE_CONST registry_type& registry() CASTLE_CONST noexcept
    {
        return registry_;
    }

private:
    registry_type registry_{};

    tick_type period_           = 0;
    tick_type counter_          = 0;
    tick_type repeat_remaining_ = 0;
    mode      mode_             = mode::periodic;
    bool      running_          = false;
};

} // namespace events
} // namespace castle

#endif // CASTLE_EVENTS_TICK_TIMER_HPP
