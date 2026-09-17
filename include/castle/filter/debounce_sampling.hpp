// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Deterministic debounce, hold, and repeat state machine for sampled boolean inputs.
 *
 * Use this header when a periodic task or interrupt samples a noisy digital signal such as a
 * button, switch, or GPIO input. The implementation uses only fixed-capacity inline storage,
 * performs no heap allocation, and reports transitions through return values or optional
 * inline callbacks stored in `castle::callbacks::function`.
 *
 * @code
 * #include "castle/filter/debounce_sampling.hpp"
 *
 * castle::filter::debounce_sampling<5U, 10U, 3U> key;
 * if (key.sample(true) && key.is_valid())
 * {
 *     // Debounced press confirmed.
 * }
 * @endcode
 */
#ifndef CASTLE_FILTER_DEBOUNCE_SAMPLING_HPP
#define CASTLE_FILTER_DEBOUNCE_SAMPLING_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/algorithm/algorithm.hpp"
#include "castle/utility/move.hpp"

#include "castle/callbacks/function.hpp"

#include <stdint.h>

namespace castle
{
namespace filter
{

/**
 * @brief Named debounce state-machine states.
 */
enum class debounce_state : uint8_t
{
    cleared = 0U, /**< No debounced active input is currently confirmed. */
    valid,        /**< `ValidCount` consecutive active samples have been confirmed. */
    held,         /**< `HoldCount` additional active samples have been confirmed after `valid`. */
    repeating     /**< `RepeatCount` additional active samples have triggered repeat mode. */
};

/**
 * @brief Debounces a sampled boolean signal with optional hold and repeat stages.
 *
 * @tparam ValidCount Consecutive identical samples required to confirm a press or release.
 * @tparam HoldCount Consecutive active samples required after `valid` to enter `held`.
 * @tparam RepeatCount Consecutive active samples required after `held` to enter or re-fire `repeating`.
 * @tparam CounterType Unsigned integral type used for the internal sample counter.
 * @tparam CallbackStorageSize Inline storage size for each callback.
 * @tparam CallbackStorageAlignment Inline storage alignment for each callback.
 *
 * @note `RepeatCount` requires `HoldCount > 0`.
 * @warning Callbacks execute synchronously in the context that calls `sample()`.
 */
template <
    size_type ValidCount,
    size_type HoldCount = 0U,
    size_type RepeatCount = 0U,
    typename CounterType = uint16_t,
    size_type CallbackStorageSize = castle::inplace_storage_reserved,
    size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class debounce_sampling
{
    static_assert(meta::is_integral<CounterType>::value && meta::is_unsigned<CounterType>::value,
                  "castle::filter::debounce_sampling: CounterType must be an unsigned integral type");

    static_assert(ValidCount > 0U,
                  "castle::filter::debounce_sampling: ValidCount must be greater than zero");

    static_assert(RepeatCount == 0U || HoldCount > 0U,
                  "castle::filter::debounce_sampling: RepeatCount requires HoldCount > 0 (repeat builds on hold)");

    static_assert(static_cast<size_type>(castle::numeric_limits<CounterType>::max()) >= castle::max3(ValidCount, HoldCount, RepeatCount),
                  "castle::filter::debounce_sampling: CounterType is too narrow for the configured thresholds");

public:
    /** @brief Alias for the state enumeration type. */
    using state_type = debounce_state;
    /** @brief Alias for the internal counter type. */
    using counter_type = CounterType;
    /** @brief Inline callback wrapper used for transition notifications. */
    using callback_type = castle::callbacks::function<void(), CallbackStorageSize, CallbackStorageAlignment>;

    /**
     * @brief Constructs a debouncer with an initial raw-sample baseline.
     *
     * @param initial_sample Initial raw sample value recorded before the first call to `sample()`.
     */
    explicit debounce_sampling(bool initial_sample = false) CASTLE_NOEXCEPT
        : last_sample_(initial_sample)
    {
    }

    debounce_sampling(CASTLE_CONST debounce_sampling&) CASTLE_DELETE;
    debounce_sampling& operator=(CASTLE_CONST debounce_sampling&) CASTLE_DELETE;

    debounce_sampling(debounce_sampling&&) CASTLE_DELETE;
    debounce_sampling& operator=(debounce_sampling&&) CASTLE_DELETE;

    /**
     * @brief Feeds one raw sample into the state machine.
     *
     * @param value Raw boolean sample value.
     * @return `true` when this call caused a state transition or repeat event; otherwise `false`.
     */
    bool sample(bool value) CASTLE_NOEXCEPT
    {
        advance_counter(value);
        return evaluate_transition(value);
    }

    /**
     * @brief Function-call shorthand for `sample()`.
     *
     * @param value Raw boolean sample value.
     * @return Result of `sample(value)`.
     */
    bool operator()(bool value) CASTLE_NOEXCEPT
    {
        return sample(value);
    }

    /**
     * @brief Resets the state machine back to `cleared`.
     *
     * @param initial_sample Raw sample baseline to store after reset.
     */
    void reset(bool initial_sample = false) CASTLE_NOEXCEPT
    {
        state_ = state_type::cleared;
        last_sample_ = initial_sample;
        counter_ = 0U;
    }

    /**
     * @brief Returns the current debounce state.
     *
     * @return Current `debounce_state`.
     */
    CASTLE_NODISCARD state_type state() CASTLE_CONST CASTLE_NOEXCEPT { return state_; }

    /**
     * @brief Returns the last raw sample value seen by the debouncer.
     *
     * @return Last raw boolean sample.
     */
    CASTLE_NODISCARD bool value() CASTLE_CONST CASTLE_NOEXCEPT { return last_sample_; }

    /**
     * @brief Tests whether the debounced signal is active.
     *
     * @return `true` in `valid`, `held`, or `repeating`; otherwise `false`.
     */
    CASTLE_NODISCARD bool is_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_ != state_type::cleared;
    }

    /**
     * @brief Tests whether the debounced signal has reached the hold stage.
     *
     * @return `true` in `held` or `repeating`; otherwise `false`.
     */
    CASTLE_NODISCARD bool is_held() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (state_ == state_type::held) || (state_ == state_type::repeating);
    }

    /**
     * @brief Tests whether the debounced signal is currently repeating.
     *
     * @return `true` only in `repeating`.
     */
    CASTLE_NODISCARD bool is_repeating() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_ == state_type::repeating;
    }

    /**
     * @brief Registers or clears the callback invoked on entry to `valid`.
     *
     * @param callback Callback to store. Passing an empty `callback_type{}` clears the registration.
     */
    void on_valid(callback_type callback) CASTLE_NOEXCEPT     { on_valid_ = CASTLE_MOVE(callback); }

    /**
     * @brief Registers or clears the callback invoked on entry to `held`.
     *
     * @param callback Callback to store. Passing an empty `callback_type{}` clears the registration.
     */
    void on_held(callback_type callback) CASTLE_NOEXCEPT       { on_held_ = CASTLE_MOVE(callback); }

    /**
     * @brief Registers or clears the callback invoked on entry to or re-fire of `repeating`.
     *
     * @param callback Callback to store. Passing an empty `callback_type{}` clears the registration.
     */
    void on_repeating(callback_type callback) CASTLE_NOEXCEPT  { on_repeating_ = CASTLE_MOVE(callback); }

    /**
     * @brief Registers or clears the callback invoked on entry to `cleared`.
     *
     * @param callback Callback to store. Passing an empty `callback_type{}` clears the registration.
     */
    void on_cleared(callback_type callback) CASTLE_NOEXCEPT    { on_cleared_ = CASTLE_MOVE(callback); }

    /**
     * @brief Removes all registered callbacks.
     */
    void clear_callbacks() CASTLE_NOEXCEPT
    {
        on_valid_ = callback_type{};
        on_held_ = callback_type{};
        on_repeating_ = callback_type{};
        on_cleared_ = callback_type{};
    }

private:
    struct rising_transition
    {
        counter_type threshold;
        state_type next_state;
    };

    /// @brief Lookup table defining the state transitions for rising signal edges.
    static CASTLE_CONSTEXPR rising_transition RISING_TRANSITIONS[4] = {
        { static_cast<counter_type>(ValidCount),  state_type::valid },
        { static_cast<counter_type>(HoldCount),   state_type::held },
        { static_cast<counter_type>(RepeatCount), state_type::repeating },
        { static_cast<counter_type>(RepeatCount), state_type::repeating }
    };

    /**
     * @brief Advances the internal counter based on the current sample value.
     * @param value The current sample value used to update the counter.
     * @note This function should be called whenever a new sample is available to update the internal counter accordingly.
     */
    void advance_counter(bool value) CASTLE_NOEXCEPT
    {
        if (value != last_sample_)
        {
            last_sample_ = value;
            counter_ = 0U;
        }

        if (counter_ < castle::numeric_limits<counter_type>::max())
        {
            ++counter_;
        }
    }

    /**
     * @brief Invokes the specified callback if it is set.
     * @param callback The callback to invoke if it is set.
     * @note This function checks if the callback is set before invoking it to avoid null function calls.
     */
    static void invoke(callback_type& callback) CASTLE_NOEXCEPT
    {
        if (callback)
        {
            callback();
        }
    }

    /**
     * @brief Enters a new state and invokes the corresponding callback.
     * @param new_state The new state to enter and for which the corresponding callback will be invoked.
     * @note This function will reset the internal counter and invoke the appropriate callback for the new state.
     */
    void enter_state(state_type new_state) CASTLE_NOEXCEPT
    {
        state_ = new_state;
        counter_ = 0U;

        switch (new_state)
        {
            case state_type::cleared:
            {
                invoke(on_cleared_);
                break;
            }
            case state_type::valid:
            {
                invoke(on_valid_);
                break;
            }
            case state_type::held:
            {
                invoke(on_held_);
                break;
            }
            case state_type::repeating:
            {
                invoke(on_repeating_);
                break;
            }
            default:
            {
                break;
            }
        }
    }

    /**
     * @brief Evaluates whether a state transition should occur based on the current sample value.
     * @param value The current sample value used to determine if a state transition should occur.
     * @return True if a state transition occurred, false otherwise.
     */
    bool evaluate_transition(bool value) CASTLE_NOEXCEPT
    {
        if (!value)
        {
            if (state_ != state_type::cleared && counter_ == static_cast<counter_type>(ValidCount))
            {
                enter_state(state_type::cleared);
                return true;
            }
            return false;
        }

        CASTLE_CONST rising_transition& next = RISING_TRANSITIONS[static_cast<size_type>(state_)];
        if (next.threshold != 0U && counter_ == next.threshold)
        {
            enter_state(next.next_state);
            return true;
        }
        return false;
    }

    state_type state_ = state_type::cleared;
    bool last_sample_ = false;
    counter_type counter_ = 0U;

    callback_type on_valid_{};
    callback_type on_held_{};
    callback_type on_repeating_{};
    callback_type on_cleared_{};
};

} // namespace filter
} // namespace castle

#endif // CASTLE_FILTER_DEBOUNCE_SAMPLING_HPP
