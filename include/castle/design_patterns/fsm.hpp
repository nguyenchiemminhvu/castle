// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file fsm.hpp
 * @brief Compile-time described, allocation-free finite state machine engine.
 *
 * States and events are represented by C++ types. A transition table maps an
 * exact (source state, event type) pair to a target state and optional guard
 * and action functors. The table is checked at compile time; each machine
 * instance stores only its current state index.
 *
 * The engine uses static dispatch only: no heap allocation, exceptions, RTTI,
 * virtual functions, or STL dependencies are required.
 *
 * @code
 * #include "castle/design_patterns/fsm.hpp"
 *
 * struct locked {};
 * struct unlocked {};
 * struct key_turned {};
 * struct door_closed {};
 *
 * using door_fsm = castle::design_patterns::fsm::state_machine<
 *     locked,
 *     castle::design_patterns::fsm::transition<locked, key_turned, unlocked>,
 *     castle::design_patterns::fsm::transition<unlocked, door_closed, locked>
 * >;
 *
 * int main()
 * {
 *     door_fsm door;
 *     door.process_event(key_turned{});
 *     return door.is_in_state<unlocked>() ? 0 : 1;
 * }
 * @endcode
 */
#ifndef CASTLE_DESIGN_PATTERNS_FSM_HPP
#define CASTLE_DESIGN_PATTERNS_FSM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

#include <stdint.h>

namespace castle
{
namespace design_patterns
{
namespace fsm
{

/**
 * @brief Context passed to guards and actions when an event is processed.
 *
 * The no-context overload of `state_machine::process_event()` creates this
 * empty object and passes it to the selected guard and action. Custom
 * functors used through that overload should therefore accept `empty_context&`.
 */
struct empty_context
{
};

/**
 * @brief Default guard that permits a transition.
 *
 * @tparam Context Context type supplied to `process_event()`.
 * @tparam Event Event type associated with the transition.
 */
struct always_guard
{
    template <typename Context, typename Event>
    bool operator()(Context&, CASTLE_CONST Event&) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return true;
    }
};

/**
 * @brief Default transition action that performs no work.
 *
 * @tparam Context Context type supplied to `process_event()`.
 * @tparam Event Event type associated with the transition.
 */
struct no_action
{
    template <typename Context, typename Event>
    void operator()(Context&, CASTLE_CONST Event&) CASTLE_CONST CASTLE_NOEXCEPT
    {
    }
};

/**
 * @brief Describes one compile-time transition table row.
 *
 * @tparam SourceState State in which the event is accepted.
 * @tparam Event Exact event type that triggers the row.
 * @tparam TargetState State entered after the guard passes and action runs.
 * @tparam Guard Default-constructible callable returning a bool-convertible
 *         value from `guard(context, event)`.
 * @tparam Action Default-constructible callable invoked as
 *         `action(context, event)` after the guard passes.
 *
 * A machine may contain only one row for each `(SourceState, Event)` pair.
 * Use the context object to hold mutable application data; guard and action
 * objects are temporary stateless policy objects constructed for each event.
 */
template <typename SourceState,
          typename Event,
          typename TargetState,
          typename Guard = always_guard,
          typename Action = no_action>
struct transition
{
    static_assert(!meta::is_void<SourceState>::value, "FSM source state must not be void.");
    static_assert(!meta::is_void<Event>::value, "FSM event type must not be void.");
    static_assert(!meta::is_void<TargetState>::value, "FSM target state must not be void.");
    static_assert(meta::is_same<SourceState, meta::decay_t<SourceState> >::value,
                  "FSM source state type must not have cv/ref qualifiers.");
    static_assert(meta::is_same<Event, meta::decay_t<Event> >::value,
                  "FSM event type must not have cv/ref qualifiers.");
    static_assert(meta::is_same<TargetState, meta::decay_t<TargetState> >::value,
                  "FSM target state type must not have cv/ref qualifiers.");
    static_assert(!meta::is_void<Guard>::value, "FSM guard type must not be void.");
    static_assert(!meta::is_void<Action>::value, "FSM action type must not be void.");
    static_assert(meta::is_same<Guard, meta::decay_t<Guard> >::value,
                  "FSM guard type must not have cv/ref qualifiers.");
    static_assert(meta::is_same<Action, meta::decay_t<Action> >::value,
                  "FSM action type must not have cv/ref qualifiers.");
    static_assert(!meta::is_pointer<meta::remove_cv_t<Guard> >::value,
                  "FSM guards must be callable object types, not function pointers.");
    static_assert(!meta::is_pointer<meta::remove_cv_t<Action> >::value,
                  "FSM actions must be callable object types, not function pointers.");

    using source_state = SourceState;
    using event_type = Event;
    using target_state = TargetState;
    using guard_type = Guard;
    using action_type = Action;
};

/** @brief Result of processing one event. */
enum class event_result : uint8_t
{
    /** A matching transition passed its guard, ran its action, and committed its target state. */
    transitioned,
    /** No transition row matched the current state and exact event type. */
    ignored,
    /** A row matched, but its guard rejected the event; state was not changed. */
    guard_rejected
};

namespace detail
{

template <typename... Types>
struct type_list
{
};

template <typename T, typename List>
struct contains;

template <typename T, typename... Types>
struct contains<T, type_list<Types...> >
    : meta::bool_constant<(meta::is_same<T, Types>::value || ... || false)>
{
};

template <typename List, typename T>
struct append_unique;

template <typename... Types, typename T>
struct append_unique<type_list<Types...>, T>
{
    using type = meta::conditional_t<contains<T, type_list<Types...> >::value,
                                     type_list<Types...>,
                                     type_list<Types..., T> >;
};

template <typename List, typename... Transitions>
struct collect_states;

template <typename List>
struct collect_states<List>
{
    using type = List;
};

template <typename List, typename Head, typename... Tail>
struct collect_states<List, Head, Tail...>
{
private:
    using with_source = typename append_unique<List, typename Head::source_state>::type;
    using with_target = typename append_unique<with_source, typename Head::target_state>::type;

public:
    using type = typename collect_states<with_target, Tail...>::type;
};

template <typename T, typename List>
struct index_of;

template <typename T, typename Head, typename... Tail>
struct index_of<T, type_list<Head, Tail...> >
    : meta::integral_constant<size_type,
          meta::is_same<T, Head>::value ? 0U : (1U + index_of<T, type_list<Tail...> >::value)>
{
};

template <typename T>
struct index_of<T, type_list<> > : meta::integral_constant<size_type, static_cast<size_type>(-1)>
{
};

template <typename T>
struct is_transition : meta::false_type
{
};

template <typename SourceState, typename Event, typename TargetState, typename Guard, typename Action>
struct is_transition<transition<SourceState, Event, TargetState, Guard, Action> > : meta::true_type
{
};

template <typename... Transitions>
struct unique_transition_keys;

template <>
struct unique_transition_keys<> : meta::true_type
{
};

template <typename Head, typename... Tail>
struct unique_transition_keys<Head, Tail...>
    : meta::bool_constant<
          ((!meta::is_same<typename Head::source_state, typename Tail::source_state>::value
             || !meta::is_same<typename Head::event_type, typename Tail::event_type>::value) && ...)
          && unique_transition_keys<Tail...>::value>
{
};

template <size_type Index, typename... Types>
struct type_at;

template <typename Head, typename... Tail>
struct type_at<0U, Head, Tail...>
{
    using type = Head;
};

template <size_type Index, typename Head, typename... Tail>
struct type_at<Index, Head, Tail...> : type_at<Index - 1U, Tail...>
{
};

template <typename List>
struct type_list_size;

template <typename... Types>
struct type_list_size<type_list<Types...> > : meta::integral_constant<size_type, sizeof...(Types)>
{
};

template <typename State, typename Event, typename... Transitions>
struct has_transition_key
    : meta::bool_constant<((meta::is_same<State, typename Transitions::source_state>::value
                            && meta::is_same<Event, typename Transitions::event_type>::value) || ... || false)>
{
};

} // namespace detail

/**
 * @brief A statically described finite state machine with deterministic event dispatch.
 *
 * @tparam InitialState State selected when a machine is constructed or reset.
 * @tparam Transitions Transition table row types (`transition<...>`).
 *
 * The set of states is inferred from `InitialState` and every row's source and
 * target types. State identity is converted to a compile-time index, while an
 * instance stores only that index. Event dispatch scans the transition rows in
 * declaration order and selects the unique row whose source state and event
 * type match.
 *
 * @note The class is not internally synchronized. Protect an instance when it
 *       is accessed from multiple threads or interrupt contexts.
 */
template <typename InitialState, typename... Transitions>
class state_machine CASTLE_FINAL
{
private:
    using state_list = typename detail::collect_states<detail::type_list<InitialState>, Transitions...>::type;

    static_assert(!meta::is_void<InitialState>::value, "FSM initial state must not be void.");
    static_assert((detail::is_transition<Transitions>::value && ...),
                  "Every FSM table entry must be an fsm::transition specialization.");
    static_assert(detail::unique_transition_keys<Transitions...>::value,
                  "FSM transition table contains duplicate (source state, event) keys.");

public:
    /** @brief Alias for the configured initial state type. */
    using initial_state = InitialState;

    /** @brief Number of unique state types inferred from the table. */
    static CASTLE_CONSTEXPR size_type state_count = detail::type_list_size<state_list>::value;

    /** @brief Number of transition table rows. */
    static CASTLE_CONSTEXPR size_type transition_count = sizeof...(Transitions);

    /**
     * @brief Constructs a machine in its initial state.
     */
    state_machine() CASTLE_NOEXCEPT
        : current_state_(detail::index_of<InitialState, state_list>::value)
    {
    }

    /**
     * @brief Returns the compile-time state index assigned to a state type.
     * @tparam State State type to query.
     * @return Stable index in this machine's inferred state list.
     */
    template <typename State>
    static CASTLE_CONSTEXPR size_type state_id() CASTLE_NOEXCEPT
    {
        static_assert(detail::contains<State, state_list>::value, "State type is not part of this FSM.");
        return detail::index_of<State, state_list>::value;
    }

    /**
     * @brief Returns the current state index.
     * @return Index corresponding to one of the state types in the transition table.
     */
    size_type current_state_id() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_state_;
    }

    /**
     * @brief Tests whether the machine currently occupies `State`.
     * @tparam State State type to test.
     * @return `true` when the current state matches `State`.
     */
    template <typename State>
    bool is_in_state() CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(detail::contains<State, state_list>::value, "State type is not part of this FSM.");
        return current_state_ == detail::index_of<State, state_list>::value;
    }

    /**
     * @brief Restores the machine to its configured initial state.
     *
     * No transition guard or action is invoked by reset().
     */
    void reset() CASTLE_NOEXCEPT
    {
        current_state_ = detail::index_of<InitialState, state_list>::value;
    }

    /**
     * @brief Checks at compile time whether a state/event pair has a table row.
     * @tparam State Candidate source state.
     * @tparam Event Candidate event type; cv/ref qualifiers are removed.
     * @return Compile-time `true` when the table contains the pair.
     */
    template <typename State, typename Event>
    static CASTLE_CONSTEXPR bool has_transition() CASTLE_NOEXCEPT
    {
        return detail::contains<State, state_list>::value
            && detail::has_transition_key<State, meta::decay_t<Event>, Transitions...>::value;
    }

    /**
     * @brief Processes an event using a caller-owned context object.
     *
     * @tparam Event Event type. Matching is exact after cv/ref removal.
     * @tparam Context Application context passed to the selected guard and action.
     * @param event Event instance delivered to the machine.
     * @param context Mutable or CASTLE_CONST application context, depending on functor signatures.
     * @return `transitioned`, `ignored`, or `guard_rejected`.
     *
     * Guards run before actions. When the guard returns false, the action is
     * skipped and the current state is unchanged. On success, the action runs
     * before the target state index is committed.
     */
    template <typename Event, typename Context>
    CASTLE_NODISCARD event_result process_event(CASTLE_CONST Event& event, Context& context) CASTLE_NOEXCEPT
    {
        using event_type = meta::decay_t<Event>;
        return process_event_impl<0U, event_type>(event, context);
    }

    /**
     * @brief Processes an event with an automatically created empty context.
     * @tparam Event Event type. Matching is exact after cv/ref removal.
     * @param event Event instance delivered to the machine.
     * @return `transitioned`, `ignored`, or `guard_rejected`.
     *
     * This overload is suitable when guards and actions do not need external
     * application data, or when they explicitly accept `empty_context&`.
     */
    template <typename Event>
    CASTLE_NODISCARD event_result process_event(CASTLE_CONST Event& event) CASTLE_NOEXCEPT
    {
        empty_context context{};
        return process_event(event, context);
    }

private:
    template <size_type Index, typename Event, typename Context>
    event_result process_event_impl(CASTLE_CONST Event& event, Context& context) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (Index >= sizeof...(Transitions))
        {
            return event_result::ignored;
        }
        else
        {
            using row = typename detail::type_at<Index, Transitions...>::type;

            CASTLE_IF_CONSTEXPR (meta::is_same<Event, typename row::event_type>::value)
            {
                if (current_state_ == detail::index_of<typename row::source_state, state_list>::value)
                {
                    using guard_type = typename row::guard_type;
                    using action_type = typename row::action_type;

                    static_assert(meta::is_nothrow_default_constructible<guard_type>::value,
                                  "FSM guard types must be nothrow default constructible.");
                    static_assert(meta::is_nothrow_default_constructible<action_type>::value,
                                  "FSM action types must be nothrow default constructible.");
                    static_assert(meta::is_invocable<guard_type&, Context&, Event CASTLE_CONST&>::value,
                                  "FSM guard must be callable as guard(context, event).");
                    static_assert(meta::is_invocable<action_type&, Context&, Event CASTLE_CONST&>::value,
                                  "FSM action must be callable as action(context, event).");
                    static_assert(noexcept(meta::declval<guard_type&>()(meta::declval<Context&>(),
                                                                        meta::declval<Event CASTLE_CONST&>())),
                                  "FSM guard invocation must be noexcept.");
                    static_assert(noexcept(meta::declval<action_type&>()(meta::declval<Context&>(),
                                                                         meta::declval<Event CASTLE_CONST&>())),
                                  "FSM action invocation must be noexcept.");
                    using guard_result_type = meta::invoke_result_t<guard_type&, Context&, Event CASTLE_CONST&>;
                    static_assert(noexcept(static_cast<bool>(meta::declval<guard_result_type>())),
                                  "FSM guard result conversion to bool must be noexcept.");

                    guard_type guard{};
                    if (!static_cast<bool>(guard(context, event)))
                    {
                        return event_result::guard_rejected;
                    }

                    action_type action{};
                    action(context, event);
                    current_state_ = detail::index_of<typename row::target_state, state_list>::value;
                    return event_result::transitioned;
                }
            }

            return process_event_impl<Index + 1U, Event>(event, context);
        }
    }

    size_type current_state_;
};

} // namespace fsm
} // namespace design_patterns
} // namespace castle

#endif // CASTLE_DESIGN_PATTERNS_FSM_HPP
