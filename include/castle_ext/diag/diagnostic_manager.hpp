// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file diagnostic_manager.hpp
 * @brief Fixed-configuration, event-driven diagnostics for embedded systems.
 *
 * A diagnostic manager receives boolean status reports, qualifies them over
 * consecutive update cycles or elapsed steady-clock time, and evaluates
 * compile-time groups of previously declared diagnostics. Recovery callbacks
 * are non-owning, allocation-free slots and are invoked only on the transition
 * into a qualified failure state.
 *
 * The manager is deliberately polled by its owner. It creates no thread, timer,
 * heap allocation, virtual dispatch, or hidden scheduling behavior.
 */
#ifndef CASTLE_EXT_DIAG_DIAGNOSTIC_MANAGER_HPP
#define CASTLE_EXT_DIAG_DIAGNOSTIC_MANAGER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/tuple.hpp"
#include "castle/chrono/clocks.hpp"

#include <stdint.h>

namespace castle
{
namespace diag
{

// ---------------------------------------------------------------------------
// Qualification and re-arming policies
// ---------------------------------------------------------------------------

/** @brief Qualify a failure on the first update where its condition is true. */
struct immediate
{
};

/**
 * @brief Qualify a failure after a number of consecutive update cycles.
 * @tparam Cycles Number of consecutive true evaluations required.
 * @note The first true evaluation counts as cycle one. A false evaluation
 *       resets the counter to zero.
 */
template <castle::size_type Cycles>
struct after_cycles
{
    static_assert(Cycles > 0U, "after_cycles requires at least one cycle");
    static CASTLE_CONSTEXPR castle::size_type value = Cycles;
};

/**
 * @brief Qualify a failure after a steady-clock duration has elapsed.
 * @tparam Duration Castle chrono duration type, e.g. castle::chrono::milliseconds.
 * @tparam Count Compile-time duration count expressed in Duration units.
 * @note The condition must remain true continuously for the configured time.
 */
template <typename Duration, int64_t Count>
struct after_duration
{
    static_assert(Count >= 0, "after_duration cannot be negative");
    using duration_type = Duration;
    static CASTLE_CONSTEXPR int64_t value = Count;
};

/**
 * @brief Automatically re-arm the recovery action after the diagnostic clears.
 * @note The action still executes only once for each qualified failure episode.
 */
struct rearm_on_clear
{
};

/**
 * @brief Do not automatically re-arm the recovery action when the diagnostic clears.
 * @note Call diagnostic_manager::rearm<Tag>() to permit another recovery action.
 */
struct one_shot
{
};

// ---------------------------------------------------------------------------
// Diagnostic rules and boolean group expressions
// ---------------------------------------------------------------------------

/**
 * @brief A status event reported by a component.
 * @tparam Tag Unique diagnostic tag type.
 * @tparam Qualification Qualification policy; defaults to immediate.
 * @tparam Rearm Recovery re-arm policy; defaults to rearm_on_clear.
 *
 * A component reports the current condition using
 * `manager.report<Tag>(condition)`. The last reported value is sampled by
 * `diagnostic_manager::update()`.
 */
template <typename Tag,
          typename Qualification = immediate,
          typename Rearm = rearm_on_clear>
struct diagnostic_event
{
    using tag_type = Tag;
    using qualification_type = Qualification;
    using rearm_type = Rearm;
    static CASTLE_CONSTEXPR bool is_group = false;
};

/** @brief Boolean expression that is true while the named diagnostic is active. */
template <typename Tag>
struct active
{
    using tag_type = Tag;
};

/** @brief Boolean expression that is true only when every child expression is true. */
template <typename... Expressions>
struct all_of
{
    static_assert(sizeof...(Expressions) > 0U, "all_of requires at least one expression");
};

/** @brief Boolean expression that is true when at least one child expression is true. */
template <typename... Expressions>
struct any_of
{
    static_assert(sizeof...(Expressions) > 0U, "any_of requires at least one expression");
};

/** @brief Boolean expression that negates one child expression. */
template <typename Expression>
struct not_ 
{
};

/**
 * @brief A composite diagnostic evaluated from other diagnostics.
 * @tparam Tag Unique diagnostic tag type.
 * @tparam Expression Boolean expression built from active, all_of, any_of, and not_.
 * @tparam Qualification Qualification policy; defaults to immediate.
 * @tparam Rearm Recovery re-arm policy; defaults to rearm_on_clear.
 *
 * Group rules are evaluated after all source events. When a group depends on
 * another group, declare the dependency first. The manager checks this ordering
 * at compile time, so groups can be safely chained without run-time graph walks.
 */
template <typename Tag,
          typename Expression,
          typename Qualification = immediate,
          typename Rearm = rearm_on_clear>
struct diagnostic_group
{
    using tag_type = Tag;
    using expression_type = Expression;
    using qualification_type = Qualification;
    using rearm_type = Rearm;
    static CASTLE_CONSTEXPR bool is_group = true;
};

// ---------------------------------------------------------------------------
// Public result types
// ---------------------------------------------------------------------------

/** @brief Summary of state transitions produced by one manager update. */
struct update_result
{
    castle::size_type activated = 0U; /**< Diagnostics that became active. */
    castle::size_type cleared = 0U; /**< Diagnostics that returned to normal. */
    castle::size_type recoveries_executed = 0U; /**< Recovery callbacks invoked. */
};

/**
 * @brief Read-only snapshot of one diagnostic's current state.
 * @note `reported` is meaningful for diagnostic_event and is always false for
 *       diagnostic_group. `consecutive_samples` is the current qualification
 *       progress and is zero when no qualification is pending.
 */
struct diagnostic_snapshot
{
    bool reported = false;
    bool active = false;
    bool pending = false;
    bool enabled = true;
    bool recovery_armed = true;
    castle::size_type consecutive_samples = 0U;
    castle::size_type activation_count = 0U;
    castle::size_type clear_count = 0U;
    castle::chrono::steady_clock::time_point last_transition{};
};

namespace detail
{

template <typename Tag, typename... Rules>
struct tag_count;

template <typename Tag>
struct tag_count<Tag> : castle::meta::integral_constant<castle::size_type, 0U>
{
};

template <typename Tag, typename Head, typename... Tail>
struct tag_count<Tag, Head, Tail...>
    : castle::meta::integral_constant<
          castle::size_type,
          (castle::meta::is_same<Tag, typename Head::tag_type>::value ? 1U : 0U)
              + tag_count<Tag, Tail...>::value>
{
};

template <typename Tag, typename... Rules>
struct tag_index;

template <typename Tag>
struct tag_index<Tag> : castle::meta::integral_constant<castle::size_type, 0U>
{
};

template <typename Tag, typename Head, typename... Tail>
struct tag_index<Tag, Head, Tail...>
    : castle::meta::integral_constant<
          castle::size_type,
          castle::meta::is_same<Tag, typename Head::tag_type>::value
              ? 0U
              : 1U + tag_index<Tag, Tail...>::value>
{
};

template <typename... Rules>
struct unique_tags;

template <>
struct unique_tags<> : castle::meta::true_type
{
};

template <typename Head, typename... Tail>
struct unique_tags<Head, Tail...>
    : castle::meta::bool_constant<
          (tag_count<typename Head::tag_type, Tail...>::value == 0U)
              && unique_tags<Tail...>::value>
{
};

template <castle::size_type CurrentIndex, typename AllRules, typename Expression>
struct expression_is_ordered;

template <typename Tag, castle::size_type CurrentIndex, typename AllRules>
struct active_reference_is_ordered;

template <typename Tag, castle::size_type CurrentIndex, typename... Rules>
struct active_reference_is_ordered<Tag, CurrentIndex, castle::tuple<Rules...>>
    : castle::meta::bool_constant<
          (tag_count<Tag, Rules...>::value == 1U)
              && (tag_index<Tag, Rules...>::value < CurrentIndex)>
{
};

template <castle::size_type CurrentIndex, typename AllRules, typename Tag>
struct expression_is_ordered<CurrentIndex, AllRules, active<Tag>>
    : active_reference_is_ordered<Tag, CurrentIndex, AllRules>
{
};

template <castle::size_type CurrentIndex, typename AllRules, typename... Expressions>
struct expression_is_ordered<CurrentIndex, AllRules, all_of<Expressions...>>
    : castle::meta::bool_constant<
          (expression_is_ordered<CurrentIndex, AllRules, Expressions>::value && ...)>
{
};

template <castle::size_type CurrentIndex, typename AllRules, typename... Expressions>
struct expression_is_ordered<CurrentIndex, AllRules, any_of<Expressions...>>
    : castle::meta::bool_constant<
          (expression_is_ordered<CurrentIndex, AllRules, Expressions>::value && ...)>
{
};

template <castle::size_type CurrentIndex, typename AllRules, typename Expression>
struct expression_is_ordered<CurrentIndex, AllRules, not_<Expression>>
    : expression_is_ordered<CurrentIndex, AllRules, Expression>
{
};

template <typename Rule, castle::size_type CurrentIndex, typename AllRules>
struct rule_dependencies_are_ordered : castle::meta::true_type
{
};

template <typename Tag, typename Expression, typename Qualification, typename Rearm,
          castle::size_type CurrentIndex, typename AllRules>
struct rule_dependencies_are_ordered<
    diagnostic_group<Tag, Expression, Qualification, Rearm>,
    CurrentIndex,
    AllRules>
    : expression_is_ordered<CurrentIndex, AllRules, Expression>
{
};

template <castle::size_type CurrentIndex, typename AllRules, typename RemainingRules>
struct ordered_rules;

template <castle::size_type CurrentIndex, typename AllRules>
struct ordered_rules<CurrentIndex, AllRules, castle::tuple<>> : castle::meta::true_type
{
};

template <castle::size_type CurrentIndex,
          typename AllRules,
          typename Head,
          typename... Tail>
struct ordered_rules<CurrentIndex, AllRules, castle::tuple<Head, Tail...>>
    : castle::meta::bool_constant<
          rule_dependencies_are_ordered<Head, CurrentIndex, AllRules>::value
              && ordered_rules<CurrentIndex + 1U,
                               AllRules,
                               castle::tuple<Tail...>>::value>
{
};

template <typename Rule>
struct monitor_state
{
    using time_point = castle::chrono::steady_clock::time_point;
    using recovery_callback = void (*)(void*) CASTLE_NOEXCEPT;

    bool reported = false;
    bool active = false;
    bool pending = false;
    bool enabled = true;
    bool recovery_armed = true;
    castle::size_type consecutive_samples = 0U;
    castle::size_type activation_count = 0U;
    castle::size_type clear_count = 0U;
    time_point qualification_started{};
    time_point last_transition{};
    recovery_callback recovery = nullptr;
    void* recovery_context = nullptr;
};

template <typename Qualification, typename TimePoint>
struct qualification_test;

template <typename TimePoint>
struct qualification_test<immediate, TimePoint>
{
    static bool satisfied(castle::size_type, CASTLE_CONST TimePoint&, CASTLE_CONST TimePoint&) CASTLE_NOEXCEPT
    {
        return true;
    }
};

template <castle::size_type Cycles, typename TimePoint>
struct qualification_test<after_cycles<Cycles>, TimePoint>
{
    static bool satisfied(castle::size_type samples, CASTLE_CONST TimePoint&, CASTLE_CONST TimePoint&) CASTLE_NOEXCEPT
    {
        return samples >= Cycles;
    }
};

template <typename Duration, int64_t Count, typename TimePoint>
struct qualification_test<after_duration<Duration, Count>, TimePoint>
{
    static bool satisfied(castle::size_type, CASTLE_CONST TimePoint& started, CASTLE_CONST TimePoint& now) CASTLE_NOEXCEPT
    {
        return (now - started) >= Duration(static_cast<typename Duration::rep>(Count));
    }
};

template <typename Expression, typename Manager>
struct expression_evaluator;

template <typename Tag, typename Manager>
struct expression_evaluator<active<Tag>, Manager>
{
    static bool evaluate(CASTLE_CONST Manager& manager) CASTLE_NOEXCEPT
    {
        return manager.template is_active<Tag>();
    }
};

template <typename... Expressions, typename Manager>
struct expression_evaluator<all_of<Expressions...>, Manager>
{
    static bool evaluate(CASTLE_CONST Manager& manager) CASTLE_NOEXCEPT
    {
        return (expression_evaluator<Expressions, Manager>::evaluate(manager) && ...);
    }
};

template <typename... Expressions, typename Manager>
struct expression_evaluator<any_of<Expressions...>, Manager>
{
    static bool evaluate(CASTLE_CONST Manager& manager) CASTLE_NOEXCEPT
    {
        return (expression_evaluator<Expressions, Manager>::evaluate(manager) || ...);
    }
};

template <typename Expression, typename Manager>
struct expression_evaluator<not_<Expression>, Manager>
{
    static bool evaluate(CASTLE_CONST Manager& manager) CASTLE_NOEXCEPT
    {
        return !expression_evaluator<Expression, Manager>::evaluate(manager);
    }
};

CASTLE_INLINE void increment_saturated(castle::size_type& value) CASTLE_NOEXCEPT
{
    if (value < static_cast<castle::size_type>(~castle::size_type(0)))
    {
        ++value;
    }
}

} // namespace detail

/**
 * @brief Central, fixed-capacity diagnostic evaluator configured by rule types.
 * @tparam Rules diagnostic_event and diagnostic_group rule types.
 *
 * The manager stores one compact state record per configured rule. Components
 * publish current status with report<Tag>(), then the application calls
 * update() at its chosen diagnostic cadence. The no-argument overload uses
 * Castle's steady_clock; the time-point overload supports deterministic tests
 * and applications with an already sampled timestamp.
 *
 * @note Every tag must be unique. Group dependencies must be declared before
 *       the group that references them. Events are evaluated first, then groups
 *       in declaration order.
 * @note All calls that mutate or update one manager must be externally
 *       serialized. The class contains no internal locks.
 */
template <typename... Rules>
class diagnostic_manager
{
private:
    using rules_type = castle::tuple<Rules...>;
    using states_type = castle::tuple<detail::monitor_state<Rules>...>;

    static_assert(sizeof...(Rules) > 0U, "diagnostic_manager requires at least one rule");
    static_assert(detail::unique_tags<Rules...>::value,
                  "diagnostic_manager rule tags must be unique");
    static_assert(detail::ordered_rules<0U, rules_type, rules_type>::value,
                  "diagnostic_group dependencies must exist and be declared before the group");

public:
    using time_point = castle::chrono::steady_clock::time_point;
    using duration = castle::chrono::steady_clock::duration;
    using recovery_callback = void (*)(void*) CASTLE_NOEXCEPT;

    static CASTLE_CONSTEXPR castle::size_type rule_count = sizeof...(Rules);

    diagnostic_manager() CASTLE_DEFAULT;

    diagnostic_manager(CASTLE_CONST diagnostic_manager&) CASTLE_DELETE;
    diagnostic_manager& operator=(CASTLE_CONST diagnostic_manager&) CASTLE_DELETE;
    diagnostic_manager(diagnostic_manager&&) CASTLE_DEFAULT;
    diagnostic_manager& operator=(diagnostic_manager&&) CASTLE_DEFAULT;

    /**
     * @brief Reports the current raw condition for a source event.
     * @tparam Tag Tag of a diagnostic_event in this manager.
     * @param condition True while the component currently reports a fault condition.
     * @return status::ok when the report has been stored.
     * @note Multiple reports before update() are coalesced; the last value wins.
     *       Use after_cycles when the condition must persist across update cycles.
     */
    template <typename Tag>
    castle::status report(bool condition) CASTLE_NOEXCEPT
    {
        using rule_type = typename castle::tuple_element<detail::tag_index<Tag, Rules...>::value,
                                                         rules_type>::type;
        static_assert(detail::tag_count<Tag, Rules...>::value == 1U,
                      "report<Tag> requires a tag configured in this manager");
        static_assert(!rule_type::is_group,
                      "report<Tag> can only target diagnostic_event rules");
        this->template state<Tag>().reported = condition;
        return castle::status::ok;
    }

    /**
     * @brief Installs a non-owning recovery callable for a diagnostic.
     * @tparam Tag Tag of a configured event or group.
     * @param callable Lvalue function object (functor or lambda) with a noexcept
     *                 nullary call operator.
     * @return status::ok when the callback slot is configured.
     * @warning The callable object must outlive this manager's registration or
     *          be cleared with clear_recovery<Tag>(). Temporary callables are
     *          rejected to prevent dangling callback contexts.
     * @note Only a function pointer and context pointer are stored; there is no
     *       heap allocation, virtual dispatch, or fixed-size callable buffer.
     */
    /**
     * @brief Installs an embedded-style recovery callback with an explicit context.
     * @param callback No-throw callback function; receives @p context.
     * @param context User-owned context pointer passed to @p callback.
     * @return status::ok when the callback slot is configured.
     * @note The callback and context are non-owning. The context must remain
     *       valid for every possible invocation. Pass nullptr to use no context.
     */
    template <typename Tag>
    castle::status set_recovery(recovery_callback callback,
                                void* context = nullptr) CASTLE_NOEXCEPT
    {
        static_assert(detail::tag_count<Tag, Rules...>::value == 1U,
                      "set_recovery<Tag> requires a tag configured in this manager");
        auto& target = this->template state<Tag>();
        target.recovery_context = context;
        target.recovery = callback;
        return castle::status::ok;
    }

    template <typename Tag, typename Callable>
    castle::status set_recovery(Callable& callable) CASTLE_NOEXCEPT
    {
        static_assert(detail::tag_count<Tag, Rules...>::value == 1U,
                      "set_recovery<Tag> requires a tag configured in this manager");
        static_assert(castle::meta::is_invocable<Callable&>::value,
                      "recovery callable must be invocable with no arguments");
        static_assert(noexcept(castle::meta::declval<Callable&>()()),
                      "recovery callable must be noexcept");

        auto& target = this->template state<Tag>();
        target.recovery_context = &callable;
        target.recovery = [](void* context) CASTLE_NOEXCEPT
        {
            (*static_cast<Callable*>(context))();
        };
        return castle::status::ok;
    }

    /** @brief Removes the configured recovery callback without changing fault state. */
    template <typename Tag>
    castle::status clear_recovery() CASTLE_NOEXCEPT
    {
        static_assert(detail::tag_count<Tag, Rules...>::value == 1U,
                      "clear_recovery<Tag> requires a tag configured in this manager");
        auto& target = this->template state<Tag>();
        target.recovery = nullptr;
        target.recovery_context = nullptr;
        return castle::status::ok;
    }

    /**
     * @brief Pauses monitoring for one diagnostic.
     * @note A paused rule keeps its confirmed active state, does not qualify new
     *       failures, and does not clear an existing failure. Pending progress
     *       is discarded. Reports may continue to update its raw input value.
     */
    template <typename Tag>
    castle::status pause() CASTLE_NOEXCEPT
    {
        static_assert(detail::tag_count<Tag, Rules...>::value == 1U,
                      "pause<Tag> requires a tag configured in this manager");
        auto& target = this->template state<Tag>();
        target.enabled = false;
        target.pending = false;
        target.consecutive_samples = 0U;
        return castle::status::ok;
    }

    /** @brief Resumes a previously paused diagnostic with fresh qualification progress. */
    template <typename Tag>
    castle::status resume() CASTLE_NOEXCEPT
    {
        static_assert(detail::tag_count<Tag, Rules...>::value == 1U,
                      "resume<Tag> requires a tag configured in this manager");
        auto& target = this->template state<Tag>();
        target.enabled = true;
        target.pending = false;
        target.consecutive_samples = 0U;
        return castle::status::ok;
    }

    /**
     * @brief Pauses all monitoring.
     * @note Raw reports are still accepted. Confirmed diagnostic states are
     *       held until monitoring resumes; pending qualification is discarded.
     */
    void pause() CASTLE_NOEXCEPT
    {
        globally_enabled_ = false;
        reset_pending_all<0U>();
    }

    /** @brief Resumes all monitoring; each rule starts qualification afresh. */
    void resume() CASTLE_NOEXCEPT
    {
        globally_enabled_ = true;
        reset_pending_all<0U>();
    }

    /**
     * @brief Manually re-arms a recovery action configured with one_shot.
     * @param Tag Diagnostic tag to re-arm.
     * @return status::ok on success, or status::invalid_argument while the
     *         diagnostic is still active.
     */
    template <typename Tag>
    castle::status rearm() CASTLE_NOEXCEPT
    {
        static_assert(detail::tag_count<Tag, Rules...>::value == 1U,
                      "rearm<Tag> requires a tag configured in this manager");
        auto& target = this->template state<Tag>();
        if (target.active)
        {
            return castle::status::invalid_argument;
        }
        target.recovery_armed = true;
        return castle::status::ok;
    }

    /** @brief Reports whether a diagnostic is currently in the qualified failure state. */
    template <typename Tag>
    bool is_active() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return this->template state<Tag>().active;
    }

    /** @brief Reports whether qualification is currently in progress. */
    template <typename Tag>
    bool is_pending() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return this->template state<Tag>().pending;
    }

    /** @brief Reports whether a diagnostic is enabled, including global pause state. */
    template <typename Tag>
    bool is_enabled() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return globally_enabled_ && this->template state<Tag>().enabled;
    }

    /** @brief Returns a read-only snapshot of one diagnostic's current state. */
    template <typename Tag>
    diagnostic_snapshot get_state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST auto& source = this->template state<Tag>();
        diagnostic_snapshot snapshot;
        snapshot.reported = source.reported;
        snapshot.active = source.active;
        snapshot.pending = source.pending;
        snapshot.enabled = globally_enabled_ && source.enabled;
        snapshot.recovery_armed = source.recovery_armed;
        snapshot.consecutive_samples = source.consecutive_samples;
        snapshot.activation_count = source.activation_count;
        snapshot.clear_count = source.clear_count;
        snapshot.last_transition = source.last_transition;
        return snapshot;
    }

    /**
     * @brief Evaluates every enabled rule using Castle's steady clock.
     * @return Counts of newly activated diagnostics, cleared diagnostics, and
     *         invoked recovery callbacks.
     */
    update_result update() CASTLE_NOEXCEPT
    {
        return this->update(castle::chrono::steady_clock::now());
    }

    /**
     * @brief Evaluates every enabled rule at an explicit steady-clock timestamp.
     * @param now Timestamp sampled from the same monotonic time base on each call.
     * @return Counts of newly activated diagnostics, cleared diagnostics, and
     *         invoked recovery callbacks.
     * @note Supplying an earlier timestamp than a previous update is outside
     *       the steady-clock contract and can delay duration-qualified events.
     */
    update_result update(CASTLE_CONST time_point& now) CASTLE_NOEXCEPT
    {
        update_result result;
        if (!globally_enabled_)
        {
            return result;
        }
        update_events<0U>(now, result);
        update_groups<0U>(now, result);
        return result;
    }

private:
    template <typename Tag>
    auto& state() CASTLE_NOEXCEPT
    {
        static_assert(detail::tag_count<Tag, Rules...>::value == 1U,
                      "diagnostic tag is not configured in this manager");
        return castle::get<detail::tag_index<Tag, Rules...>::value>(states_);
    }

    template <typename Tag>
    CASTLE_CONST auto& state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(detail::tag_count<Tag, Rules...>::value == 1U,
                      "diagnostic tag is not configured in this manager");
        return castle::get<detail::tag_index<Tag, Rules...>::value>(states_);
    }

    template <typename Rule>
    void evaluate_rule(detail::monitor_state<Rule>& target,
                       bool condition,
                       CASTLE_CONST time_point& now,
                       update_result& result) CASTLE_NOEXCEPT
    {
        if (!target.enabled)
        {
            return;
        }

        if (!condition)
        {
            target.pending = false;
            target.consecutive_samples = 0U;
            if (target.active)
            {
                target.active = false;
                target.last_transition = now;
                detail::increment_saturated(target.clear_count);
                detail::increment_saturated(result.cleared);
                if (castle::meta::is_same<typename Rule::rearm_type, rearm_on_clear>::value)
                {
                    target.recovery_armed = true;
                }
            }
            return;
        }

        if (target.active)
        {
            return;
        }

        if (!target.pending)
        {
            target.pending = true;
            target.qualification_started = now;
            target.consecutive_samples = 0U;
        }
        detail::increment_saturated(target.consecutive_samples);

        if (detail::qualification_test<typename Rule::qualification_type, time_point>::satisfied(
                target.consecutive_samples, target.qualification_started, now))
        {
            target.active = true;
            target.pending = false;
            target.consecutive_samples = 0U;
            target.last_transition = now;
            detail::increment_saturated(target.activation_count);
            detail::increment_saturated(result.activated);

            // Disarm even when no callback is installed: installing one during
            // an already-active episode must not cause a late/repeated action.
            if (target.recovery_armed)
            {
                target.recovery_armed = false;
                if (target.recovery != nullptr)
                {
                    target.recovery(target.recovery_context);
                    detail::increment_saturated(result.recoveries_executed);
                }
            }
        }
    }

    template <castle::size_type Index>
    void update_events(CASTLE_CONST time_point& now, update_result& result) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (Index < sizeof...(Rules))
        {
            using rule_type = typename castle::tuple_element<Index, rules_type>::type;
            CASTLE_IF_CONSTEXPR (!rule_type::is_group)
            {
                auto& target = castle::get<Index>(states_);
                this->evaluate_rule<rule_type>(target, target.reported, now, result);
            }
            update_events<Index + 1U>(now, result);
        }
    }

    template <castle::size_type Index>
    void update_groups(CASTLE_CONST time_point& now, update_result& result) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (Index < sizeof...(Rules))
        {
            using rule_type = typename castle::tuple_element<Index, rules_type>::type;
            CASTLE_IF_CONSTEXPR (rule_type::is_group)
            {
                auto& target = castle::get<Index>(states_);
                CASTLE_CONST bool condition = detail::expression_evaluator<
                    typename rule_type::expression_type,
                    diagnostic_manager>::evaluate(*this);
                this->evaluate_rule<rule_type>(target, condition, now, result);
            }
            update_groups<Index + 1U>(now, result);
        }
    }

    template <castle::size_type Index>
    void reset_pending_all() CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (Index < sizeof...(Rules))
        {
            auto& target = castle::get<Index>(states_);
            target.pending = false;
            target.consecutive_samples = 0U;
            reset_pending_all<Index + 1U>();
        }
    }

    states_type states_{};
    bool globally_enabled_ = true;
};

template <typename... Rules>
CASTLE_CONSTEXPR castle::size_type diagnostic_manager<Rules...>::rule_count;

} // namespace diag
} // namespace castle

#endif // CASTLE_EXT_DIAG_DIAGNOSTIC_MANAGER_HPP
