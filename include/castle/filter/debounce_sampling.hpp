#ifndef CASTLE_FILTER_DEBOUNCE_SAMPLING_HPP
#define CASTLE_FILTER_DEBOUNCE_SAMPLING_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/algorithm/algorithm.hpp"
#include "castle/utility/move.hpp"

#include "castle/callbacks/inplace_function.hpp"

#include <stdint.h>

namespace castle
{
namespace filter
{

// ============================================================================
// debounce_state
// ----------------------------------------------------------------------------
// Named FSM states for debounce_sampling. Ordered so that "deeper" states
// (reached only through a longer run of identical samples) compare greater,
// which keeps is_held()/is_valid() simple monotonic comparisons instead of
// hand-rolled bitmasks.
// ============================================================================
enum class debounce_state : uint8_t
{
    cleared = 0U,   // fewer than ValidCount consecutive "true" samples observed
    valid,          // ValidCount consecutive identical samples reached
    held,           // additionally HoldCount samples observed since becoming valid
    repeating       // additionally RepeatCount samples observed since becoming held
};

// ============================================================================
// debounce_sampling
// ----------------------------------------------------------------------------
// Deterministic, allocation-free signal debouncer modelled as an explicit
// finite state machine:
//
//     cleared --[ValidCount true samples]--> valid
//     valid   --[HoldCount  true samples]--> held        (skipped if HoldCount == 0)
//     held    --[RepeatCount true samples]--> repeating  (skipped if RepeatCount == 0)
//     repeating --[RepeatCount true samples]--> repeating (re-fires periodically)
//
//     any state --[ValidCount false samples]--> cleared
//
// A sample that differs from the previously observed value restarts the
// counter (but NOT the state) - this is what gives a debounced *release* the
// same ValidCount-sample confirmation delay as a debounced *press*, instead
// of collapsing back to `cleared` on a single opposite-polarity glitch.
//
// Every state transition optionally invokes a user-supplied void() callback
// (on_valid / on_held / on_repeating / on_cleared), stored in
// castle::callbacks::inplace_function - no heap, no virtual dispatch, no-op
// when left unset.
//
// Template parameters:
//   ValidCount                 - consecutive identical samples required to become valid (> 0)
//   HoldCount                  - additional samples (since valid) required to become held (0 = disabled)
//   RepeatCount                - additional samples (since held) required to (re-)enter repeating (0 = disabled)
//   CounterType                - unsigned integral counter storage (default uint16_t)
//   CallbackStorageSize        - inplace_function SBO size for each of the 4 callbacks
//   CallbackStorageAlignment   - inplace_function SBO alignment for each of the 4 callbacks
// ============================================================================
template <
    size_type ValidCount,
    size_type HoldCount = 0U,
    size_type RepeatCount = 0U,
    typename CounterType = uint16_t,
    size_type CallbackStorageSize = castle::inplace_storage_reserved,
    size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class debounce_sampling
{
    static_assert(castle::is_integral<CounterType>::value && castle::is_unsigned<CounterType>::value,
                  "castle::filter::debounce_sampling: CounterType must be an unsigned integral type");

    static_assert(ValidCount > 0U,
                  "castle::filter::debounce_sampling: ValidCount must be greater than zero");

    static_assert(RepeatCount == 0U || HoldCount > 0U,
                  "castle::filter::debounce_sampling: RepeatCount requires HoldCount > 0 (repeat builds on hold)");

    static_assert(static_cast<size_type>(castle::numeric_limits<CounterType>::max()) >= castle::max3(ValidCount, HoldCount, RepeatCount),
                  "castle::filter::debounce_sampling: CounterType is too narrow for the configured thresholds");

public:
    using state_type = debounce_state;
    using counter_type = CounterType;
    using callback_type = castle::callbacks::inplace_function<void(), CallbackStorageSize, CallbackStorageAlignment>;

    explicit debounce_sampling(bool initial_sample = false) CASTLE_NOEXCEPT
        : last_sample_(initial_sample)
    {
    }

    debounce_sampling(CASTLE_CONST debounce_sampling&) CASTLE_DELETE;
    debounce_sampling& operator=(CASTLE_CONST debounce_sampling&) CASTLE_DELETE;

    debounce_sampling(debounce_sampling&&) CASTLE_DELETE;
    debounce_sampling& operator=(debounce_sampling&&) CASTLE_DELETE;

    // -------------------------------------------------------------------------
    // Adds a new sample. Returns true if a state transition (or a periodic
    // repeat re-trigger) occurred on this call; the corresponding on_*
    // callback (if any) has already been invoked by the time this returns.
    // -------------------------------------------------------------------------
    bool sample(bool value) CASTLE_NOEXCEPT
    {
        advance_counter(value);
        return evaluate_transition(value);
    }

    bool operator()(bool value) CASTLE_NOEXCEPT
    {
        return sample(value);
    }

    // Restarts the FSM to `cleared` with the given sample as the new baseline.
    void reset(bool initial_sample = false) CASTLE_NOEXCEPT
    {
        state_ = state_type::cleared;
        last_sample_ = initial_sample;
        counter_ = 0U;
    }

    CASTLE_NODISCARD state_type state() CASTLE_CONST CASTLE_NOEXCEPT { return state_; }
    CASTLE_NODISCARD bool value() CASTLE_CONST CASTLE_NOEXCEPT { return last_sample_; }

    CASTLE_NODISCARD bool is_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_ != state_type::cleared;
    }

    CASTLE_NODISCARD bool is_held() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (state_ == state_type::held) || (state_ == state_type::repeating);
    }

    CASTLE_NODISCARD bool is_repeating() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_ == state_type::repeating;
    }

    // -------------------------------------------------------------------------
    // Optional per-transition callbacks. Assigning an empty callback_type{}
    // removes a previously registered callback.
    // -------------------------------------------------------------------------
    void on_valid(callback_type callback) CASTLE_NOEXCEPT     { on_valid_ = CASTLE_MOVE(callback); }
    void on_held(callback_type callback) CASTLE_NOEXCEPT       { on_held_ = CASTLE_MOVE(callback); }
    void on_repeating(callback_type callback) CASTLE_NOEXCEPT  { on_repeating_ = CASTLE_MOVE(callback); }
    void on_cleared(callback_type callback) CASTLE_NOEXCEPT    { on_cleared_ = CASTLE_MOVE(callback); }

    void clear_callbacks() CASTLE_NOEXCEPT
    {
        on_valid_ = callback_type{};
        on_held_ = callback_type{};
        on_repeating_ = callback_type{};
        on_cleared_ = callback_type{};
    }

private:
    // Per-state "rising" transition: consecutive true samples required to
    // leave that state (0 = disabled, e.g. HoldCount/RepeatCount == 0) and the
    // state entered once the threshold is reached. Indexed by state_type, so
    // adding a state later means adding one row here rather than another
    // switch case; `held`/`repeating` share RepeatCount by design (repeating
    // is `held`'s self-loop).
    struct rising_transition
    {
        counter_type threshold;
        state_type next_state;
    };

    static CASTLE_CONSTEXPR rising_transition RISING_TRANSITIONS[4] = {
        { static_cast<counter_type>(ValidCount),  state_type::valid },      // cleared -> valid
        { static_cast<counter_type>(HoldCount),   state_type::held },       // valid -> held
        { static_cast<counter_type>(RepeatCount), state_type::repeating },  // held -> repeating
        { static_cast<counter_type>(RepeatCount), state_type::repeating }   // repeating -> repeating (re-fires)
    };

    // Restarts the run-length counter whenever the raw sample flips, then
    // (saturating) counts consecutive identical samples otherwise.
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

    static void invoke(callback_type& callback) CASTLE_NOEXCEPT
    {
        if (callback)
        {
            callback();
        }
    }

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

    // A false run of ValidCount samples clears any non-cleared state; a true
    // run advances (or, from `repeating`, re-fires) via RISING_TRANSITIONS.
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