// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file action_sequence.hpp
 * @brief Fixed-capacity, self-contained sequence of one-shot calls and delays (cocos2d-x `Sequence`).
 *
 * An `action_sequence` owns an ordered list of steps. A *call* step runs a callable once and takes no
 * time; a *delay* step waits for a `castle::chrono::duration`. The owner advances time explicitly
 * with `update(elapsed)`, so behavior is deterministic: the sequence never reads a clock itself and
 * can be driven from a main loop, a `castle::events::tick_timer` callback, or differences of
 * `castle::chrono::steady_clock::now()`. This is the CASTLE counterpart of:
 * @code
 * Sequence::create(CallFunc::create(a), DelayTime::create(1.5f), CallFunc::create(b), nullptr);
 * @endcode
 *
 * The time resolution is the `Duration` template parameter (default `castle::chrono::milliseconds`).
 * Delays and elapsed times of any coarser-or-equal unit convert exactly (for example seconds into a
 * millisecond sequence); finer units are rejected at compile time because truncating them would make
 * delays fire early or let small `update()` steps be lost.
 *
 * Steps are stored in a `castle::container::array`; callables are stored in
 * `castle::callbacks::function` wrappers, so the sequence owns every captured state inline. There is no
 * heap allocation, no exception, and no RTTI. Sequences compose with `append()`, which copies the steps
 * of another sequence (flattening, so the destination never refers back to the source).
 *
 * @code
 * #include "castle/patterns/action_sequence.hpp"
 *
 * using castle::chrono::milliseconds;
 * using castle::chrono::seconds;
 *
 * castle::patterns::action_sequence<4U> sequence;
 * sequence.add_call([]() noexcept { });   // runs on the first update()
 * sequence.add_delay(seconds(1));         // waits 1000 ms
 * sequence.add_call([]() noexcept { });   // runs once 1 s has elapsed
 * sequence.start();
 * sequence.update();                      // first call runs, delay begins
 * sequence.update(milliseconds(1000));    // delay ends, second call runs, sequence completes
 * @endcode
 */
#ifndef CASTLE_PATTERNS_ACTION_SEQUENCE_HPP
#define CASTLE_PATTERNS_ACTION_SEQUENCE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/chrono/duration.hpp"
#include "castle/container/array.hpp"
#include "castle/error/status.hpp"
#include "castle/math/ratio.hpp"
#include "castle/utility/forward.hpp"

#include <stdint.h>

namespace castle
{
namespace patterns
{

/**
 * @brief Execution state of an `action_sequence`.
 */
enum class action_sequence_state : uint8_t
{
    idle,      /**< Not started, or reset/modified since the last run. */
    running,   /**< Started and waiting for more time or for the next `update()`. */
    completed  /**< Every step has run. */
};

namespace detail
{
template <typename T>
struct is_chrono_duration : meta::false_type
{
};

template <typename Rep, typename Period>
struct is_chrono_duration<chrono::duration<Rep, Period>> : meta::true_type
{
};
} // namespace detail

/**
 * @brief Compile-time bounded list of calls and delays executed in order.
 *
 * @tparam MaxActions Maximum number of steps (calls plus delays); must be non-zero.
 * @tparam Duration `castle::chrono::duration` with an integral representation; its period is the
 *         time resolution of the sequence. Use for example `chrono::duration<uint32_t, castle::milli>`
 *         on 32-bit targets, or `chrono::steady_clock::duration` when driving the sequence from
 *         `steady_clock::now()` differences.
 * @tparam CallbackStorageSize Inline bytes reserved for each stored callable.
 * @tparam CallbackStorageAlignment Alignment of that inline storage.
 *
 * Call steps consume no time. When a delay expires, its leftover time carries into the following
 * steps, so one large `update()` can run several calls and delays. Each `update()` visits every
 * step at most once, which bounds its cost by `MaxActions`.
 *
 * Steps can only be added while the sequence is not running. Callables must not modify, restart, or
 * destroy the sequence that is executing them; `start()`, `reset()`, `clear()`, `add_*()` and
 * `append()` report `status::invalid_config` when called from inside a step.
 *
 * @note No internal synchronization; drive one sequence from one execution context.
 */
template <
    size_type MaxActions,
    typename Duration = chrono::milliseconds,
    size_type CallbackStorageSize = castle::inplace_storage_reserved,
    size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class action_sequence
{
    static_assert(MaxActions > 0U, "action_sequence requires a positive capacity");
    static_assert(detail::is_chrono_duration<Duration>::value,
                  "action_sequence Duration must be a castle::chrono::duration");
    static_assert(meta::is_integral<typename Duration::rep>::value &&
                      !meta::is_same<typename Duration::rep, bool>::value,
                  "action_sequence Duration must have a non-bool integral representation");

    using rep = typename Duration::rep;

public:
    using duration_type = Duration;
    using callback_type = callbacks::function<void(), CallbackStorageSize, CallbackStorageAlignment>;
    using state_type = action_sequence_state;

    action_sequence() CASTLE_DEFAULT;

    /**
     * @brief Appends a one-shot call step.
     * @param callable Callable invocable as `void()`; copied or moved into the sequence.
     * @return `status::ok`; `status::full` when `MaxActions` steps exist; `status::invalid_config`
     *         while running or executing; `status::invalid_callback` for a null function pointer or an
     *         empty `castle::callbacks::function`.
     * @warning The callable must be copy-constructible, fit the inline storage, and not throw.
     */
    template <typename Callable>
    status add_call(Callable&& callable)
    {
        static_assert(meta::is_copy_constructible<meta::decay_t<Callable>>::value,
                      "action_sequence callables must be copy-constructible");
        static_assert(meta::is_invocable<meta::decay_t<Callable>&>::value,
                      "action_sequence callables must be invocable with no arguments");

        if (!is_editable())
        {
            return status::invalid_config;
        }

        if (size_ >= MaxActions)
        {
            return status::full;
        }

        // Emptiness must be checked before wrapping: a non-empty wrapper may hold an empty one.
        if (is_empty_callable(callable))
        {
            return status::invalid_callback;
        }

        step& target = steps_[size_];
        target.callback = callback_type{CASTLE_FORWARD<Callable>(callable)};
        target.delay = static_cast<rep>(0);
        ++size_;
        rewind();
        return status::ok;
    }

    /**
     * @brief Appends a delay step.
     * @param delay Any `castle::chrono::duration` whose unit is a whole multiple of `Duration`'s
     *        unit; zero is allowed and completes immediately.
     * @return `status::ok`; `status::full`; `status::invalid_config` while running or executing;
     *         `status::invalid_argument` for a negative delay; `status::out_of_range` when the delay
     *         does not fit `Duration`.
     */
    template <typename Rep2, typename Period2>
    status add_delay(chrono::duration<Rep2, Period2> CASTLE_CONST& delay)
    {
        if (!is_editable())
        {
            return status::invalid_config;
        }

        if (size_ >= MaxActions)
        {
            return status::full;
        }

        rep count = static_cast<rep>(0);
        CASTLE_CONST status converted = to_count(delay, count);
        if (converted != status::ok)
        {
            return converted;
        }

        step& target = steps_[size_];
        target.callback = nullptr;
        target.delay = count;
        ++size_;
        rewind();
        return status::ok;
    }

    /**
     * @brief Appends a copy of every step of another sequence (composition).
     * @tparam OtherCapacity Capacity of the source sequence.
     * @param other Source with the same duration type and callable storage configuration.
     * @return `status::ok`; `status::full` (leaving this sequence unchanged) when the steps do not
     *         fit; `status::invalid_config` when either sequence is running or executing.
     */
    template <size_type OtherCapacity>
    status append(action_sequence<OtherCapacity, Duration, CallbackStorageSize, CallbackStorageAlignment> CASTLE_CONST& other)
    {
        if (!is_editable() || !other.is_editable())
        {
            return status::invalid_config;
        }

        if (other.size_ > (MaxActions - size_))
        {
            return status::full;
        }

        for (size_type i = 0U; i < other.size_; ++i)
        {
            step& target = steps_[size_];
            target.callback = other.steps_[i].callback;
            target.delay = other.steps_[i].delay;
            ++size_;
        }

        rewind();
        return status::ok;
    }

    /**
     * @brief Starts, or restarts, the sequence from its first step without executing any step.
     * @return `status::ok`, or `status::invalid_config` when called from inside a step.
     * @note An empty sequence becomes `completed`. Steps execute on the next `update()`.
     */
    status start() CASTLE_NOEXCEPT
    {
        if (updating_)
        {
            return status::invalid_config;
        }

        rewind();
        state_ = (size_ == 0U) ? state_type::completed : state_type::running;
        return status::ok;
    }

    /**
     * @brief Runs every step reachable without time (calls and zero delays).
     * @return The state after processing.
     */
    state_type update()
    {
        return advance(static_cast<rep>(0));
    }

    /**
     * @brief Advances time and executes every step that becomes reachable.
     * @param elapsed Time since the previous call, in any unit that is a whole multiple of
     *        `Duration`'s unit. Negative values count as zero; values beyond `Duration`'s range
     *        saturate (they complete every pending delay anyway).
     * @return The state after processing. A sequence that is not running is returned unchanged.
     */
    template <typename Rep2, typename Period2>
    state_type update(chrono::duration<Rep2, Period2> CASTLE_CONST& elapsed)
    {
        rep count = static_cast<rep>(0);
        CASTLE_CONST status converted = to_count(elapsed, count);
        if (converted == status::out_of_range)
        {
            count = numeric_limits<rep>::max();
        }
        return advance(count);
    }

    /**
     * @brief Returns to `idle` and rewinds the run; steps are kept.
     * @return `status::ok`, or `status::invalid_config` when called from inside a step.
     */
    status reset() CASTLE_NOEXCEPT
    {
        if (updating_)
        {
            return status::invalid_config;
        }

        rewind();
        return status::ok;
    }

    /**
     * @brief Removes every step and returns to `idle`.
     * @return `status::ok`, or `status::invalid_config` when called from inside a step.
     */
    status clear() CASTLE_NOEXCEPT
    {
        if (updating_)
        {
            return status::invalid_config;
        }

        for (size_type i = 0U; i < size_; ++i)
        {
            steps_[i].callback = nullptr;
        }
        size_ = 0U;
        rewind();
        return status::ok;
    }

    /** @brief Returns the number of steps. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_;
    }

    /** @brief Returns whether the sequence has no steps. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == 0U;
    }

    /** @brief Returns the current execution state. */
    state_type state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_;
    }

    /** @brief Returns whether the sequence is running. */
    bool is_running() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_ == state_type::running;
    }

    /** @brief Returns whether the sequence has completed. */
    bool is_completed() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return state_ == state_type::completed;
    }

    /** @brief Returns the index of the next step to execute. */
    size_type next_index() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return next_index_;
    }

    /** @brief Returns the time left in the current delay, or zero when the next step is not a delay. */
    Duration remaining_delay() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Duration(state_ == state_type::running ? remaining_ : static_cast<rep>(0));
    }

    /** @brief Returns the compile-time maximum number of steps. */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return MaxActions;
    }

private:
    template <size_type, typename, size_type, size_type>
    friend class action_sequence;

    // A step is a delay exactly when it has no callback: add_call() never stores an empty one.
    struct step
    {
        callback_type callback{};
        rep delay = static_cast<rep>(0);
    };

    bool is_editable() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !updating_ && state_ != state_type::running;
    }

    // Only one delay can be in progress, so one counter tracks it; rewinding is O(1).
    void enter(size_type index) CASTLE_NOEXCEPT
    {
        next_index_ = index;
        remaining_ = (index < size_ && !steps_[index].callback) ? steps_[index].delay : static_cast<rep>(0);
    }

    void rewind() CASTLE_NOEXCEPT
    {
        enter(0U);
        state_ = state_type::idle;
    }

    state_type advance(rep elapsed)
    {
        if (state_ != state_type::running || updating_)
        {
            return state_;
        }

        updating_ = true;
        while (next_index_ < size_)
        {
            step& current = steps_[next_index_];
            if (current.callback)
            {
                enter(next_index_ + 1U);
                current.callback();
            }
            else if (elapsed < remaining_)
            {
                remaining_ = static_cast<rep>(remaining_ - elapsed);
                updating_ = false;
                return state_;
            }
            else
            {
                elapsed = static_cast<rep>(elapsed - remaining_);
                enter(next_index_ + 1U);
            }
        }
        updating_ = false;

        state_ = state_type::completed;
        return state_;
    }

    // Exact conversion into this sequence's unit; finer source units do not compile.
    template <typename Rep2, typename Period2>
    static status to_count(chrono::duration<Rep2, Period2> CASTLE_CONST& value, rep& out) CASTLE_NOEXCEPT
    {
        using scale = math::ratio_divide_t<Period2, typename Duration::period>;
        static_assert(meta::is_integral<Rep2>::value && !meta::is_same<Rep2, bool>::value,
                      "action_sequence times must have a non-bool integral representation");
        static_assert(scale::den == 1,
                      "Time unit is finer than the sequence resolution; convert explicitly, "
                      "e.g. castle::chrono::ceil<duration_type>(value)");

        CASTLE_IF_CONSTEXPR (meta::is_signed<Rep2>::value)
        {
            if (value.count() < static_cast<Rep2>(0))
            {
                return status::invalid_argument;
            }
        }

        CASTLE_CONST uintmax_t magnitude = static_cast<uintmax_t>(value.count());
        CASTLE_CONST uintmax_t limit =
            static_cast<uintmax_t>(numeric_limits<rep>::max()) / static_cast<uintmax_t>(scale::num);
        if (magnitude > limit)
        {
            return status::out_of_range;
        }

        out = static_cast<rep>(magnitude * static_cast<uintmax_t>(scale::num));
        return status::ok;
    }

    template <typename Callable>
    static bool is_empty_callable(Callable CASTLE_CONST&) CASTLE_NOEXCEPT
    {
        return false;
    }

    template <typename Pointee>
    static bool is_empty_callable(Pointee* callable) CASTLE_NOEXCEPT
    {
        return callable == nullptr;
    }

    template <typename Signature, size_type Size, size_type Alignment>
    static bool is_empty_callable(callbacks::function<Signature, Size, Alignment> CASTLE_CONST& callable) CASTLE_NOEXCEPT
    {
        return !callable;
    }

    container::array<step, MaxActions> steps_{};
    size_type size_ = 0U;
    size_type next_index_ = 0U;
    rep remaining_ = static_cast<rep>(0);
    state_type state_ = state_type::idle;
    bool updating_ = false;
};

} // namespace patterns
} // namespace castle

#endif // CASTLE_PATTERNS_ACTION_SEQUENCE_HPP
