// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file chain_of_responsibility.hpp
 * @brief Predicate-gated Chain of Responsibility with a compile-time bounded handler list.
 *
 * A chain is an ordered list of handlers. Every handler pairs an execution condition (predicate)
 * with a behavior (action). Processing walks the list in order:
 * - predicate true: the action runs. A `void` action (or `handler_flow::continue_chain`) moves on to
 *   the next handler; `handler_flow::stop_chain` ends the chain (`chain_result::stopped`).
 * - predicate false: the handler's `reject_policy` decides. `reject_policy::stop` (the default of
 *   `make_handler()`) ends the chain (`chain_result::rejected`), so a predicate acts as a guard for
 *   every later handler. `reject_policy::skip` (`make_skippable_handler()`) just moves on to the next handler,
 *   which gives classic "first matching handler" dispatch together with `stop_chain`.
 *
 * If no handler ends the chain, `process()` returns `chain_result::completed`. The caller manages only
 * the chain object.
 *
 * Two flavors share the same handler semantics:
 * - `chain_of_responsibility<Handlers...>`: handlers are fixed by the chain type, stored by value in
 *   a `castle::tuple`, and dispatched with a compile-time fold. No type erasure, no function pointers,
 *   and the compiler can inline every predicate and action.
 * - `bounded_chain_of_responsibility<MaxHandlers, void(Args...)>`: up to `MaxHandlers` handlers are
 *   registered at runtime into a `castle::dsa::array` of `castle::callbacks::function` slots
 *   (one slot per handler). Handlers may have different callable types as long as they fit the
 *   inline storage.
 *
 * Neither flavor uses inheritance, virtual functions, RTTI, or dynamic allocation.
 *
 * Predicates and actions receive the arguments given to `process()` as lvalues, in the same order.
 * Pass requests and contexts as references (or small value types); each handler sees the same objects.
 *
 * @code
 * #include "castle/patterns/chain_of_responsibility.hpp"
 *
 * namespace dp = castle::patterns;
 *
 * struct request { unsigned code; };
 * struct stats { unsigned seen = 0U; unsigned handled = 0U; unsigned ignored = 0U; };
 *
 * auto chain = dp::make_chain(
 *     // Step that runs for every request and never rejects.
 *     dp::make_default_handler([](request const&, stats& s) noexcept { ++s.seen; }),
 *     // Dispatch: skipped when the code differs, ends the chain when it matches.
 *     dp::make_skippable_handler(
 *         [](request const& r, stats&) noexcept { return r.code == 7U; },
 *         [](request const&, stats& s) noexcept
 *         {
 *             ++s.handled;
 *             return dp::handler_flow::stop_chain;
 *         }),
 *     dp::make_default_handler(
 *         [](request const&, stats& s) noexcept { ++s.ignored; }));
 *
 * request req{7U};
 * stats counters;
 * chain.process(req, counters); // seen == 1, handled == 1, ignored == 0 -> chain_result::stopped
 * @endcode
 */
#ifndef CASTLE_PATTERNS_CHAIN_OF_RESPONSIBILITY_HPP
#define CASTLE_PATTERNS_CHAIN_OF_RESPONSIBILITY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/dsa/array.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/integral_sequence.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/tuple.hpp"

#include <stdint.h>

namespace castle
{
namespace patterns
{

/**
 * @brief Decision returned by an action after its predicate accepted the request.
 *
 * A `void` action is equivalent to returning `continue_chain`.
 */
enum class handler_flow : uint8_t
{
    continue_chain, /**< Try the next handler in the chain. */
    stop_chain      /**< The request is handled; skip every remaining handler. */
};

/**
 * @brief What a handler does when its predicate returns false.
 */
enum class reject_policy : uint8_t
{
    stop, /**< End the chain with `chain_result::rejected`; later handlers are not evaluated. */
    skip  /**< Ignore this handler and try the next one. */
};

/**
 * @brief Outcome of one `process()` call.
 */
enum class chain_result : uint8_t
{
    completed, /**< Every handler ran or was skipped; nothing ended the chain. */
    stopped,   /**< An action returned `handler_flow::stop_chain`. */
    rejected   /**< A predicate was false on a handler with `reject_policy::stop`. */
};

/**
 * @brief Predicate that accepts every request; use it for fallback handlers.
 */
struct always_true
{
    /** @brief Accepts any argument list. */
    template <typename... Args>
    CASTLE_CONSTEXPR bool operator()(Args&&...) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return true;
    }
};

/**
 * @brief One handler: an execution condition paired with a behavior.
 *
 * @tparam Predicate Callable invocable as `bool(Args&...)`.
 * @tparam Action Callable invocable as `void(Args&...)` (implies `continue_chain`) or
 *         `handler_flow(Args&...)`.
 * @tparam OnReject Behavior when the predicate is false; `reject_policy::stop` by default.
 *
 * Both callables are stored by value; their concrete types stay visible to the compiler.
 */
template <typename Predicate, typename Action, reject_policy OnReject = reject_policy::stop>
class conditional_handler
{
public:
    using predicate_type = Predicate;
    using action_type = Action;
    static CASTLE_CONSTEXPR reject_policy on_reject = OnReject;

    conditional_handler() CASTLE_DEFAULT;

    /**
     * @brief Constructs a handler from its predicate and action.
     * @param predicate Condition evaluated for every request reaching this handler.
     * @param action Behavior executed only when the predicate returns true.
     */
    template <typename PredicateArg, typename ActionArg>
    conditional_handler(PredicateArg&& predicate, ActionArg&& action)
        : predicate_(CASTLE_FORWARD<PredicateArg>(predicate)),
          action_(CASTLE_FORWARD<ActionArg>(action))
    {
    }

    /**
     * @brief Tests the predicate and, when it accepts, runs the action.
     * @param args Arguments shared by the whole chain.
     * @return `completed` when the next handler should run (action finished or predicate false with
     *         `reject_policy::skip`); `stopped` when the action asked to stop; `rejected` when the
     *         predicate is false with `reject_policy::stop`.
     */
    template <typename... Args>
    chain_result process(Args&... args)
    {
        static_assert(meta::is_invocable_r<bool, Predicate&, Args&...>::value,
                      "Chain predicate must be callable as bool(Args&...)");
        static_assert(meta::is_invocable<Action&, Args&...>::value,
                      "Chain action must be callable with Args&...");

        using action_result = meta::invoke_result_t<Action&, Args&...>;
        static_assert(meta::is_void<action_result>::value ||
                          meta::is_same<action_result, handler_flow>::value,
                      "Chain action must return void or castle::patterns::handler_flow");

        if (!predicate_(args...))
        {
            return OnReject == reject_policy::stop ? chain_result::rejected : chain_result::completed;
        }

        CASTLE_IF_CONSTEXPR (meta::is_void<action_result>::value)
        {
            action_(args...);
            return chain_result::completed;
        }
        else
        {
            return action_(args...) == handler_flow::stop_chain ? chain_result::stopped
                                                                : chain_result::completed;
        }
    }

    /** @brief Returns the stored predicate. */
    Predicate& predicate() CASTLE_NOEXCEPT
    {
        return predicate_;
    }

    /** @brief Returns the stored action. */
    Action& action() CASTLE_NOEXCEPT
    {
        return action_;
    }

private:
    Predicate predicate_{};
    Action action_{};
};

/**
 * @brief Creates a handler that ends the chain (`rejected`) when its predicate is false.
 * @param predicate Condition of the handler; acts as a guard for the rest of the chain.
 * @param action Behavior of the handler.
 */
template <typename Predicate, typename Action>
conditional_handler<meta::decay_t<Predicate>, meta::decay_t<Action>>
make_handler(Predicate&& predicate, Action&& action)
{
    return conditional_handler<meta::decay_t<Predicate>, meta::decay_t<Action>>(
        CASTLE_FORWARD<Predicate>(predicate),
        CASTLE_FORWARD<Action>(action));
}

/**
 * @brief Creates a handler that is skipped when its predicate is false.
 * @param predicate Condition of the handler.
 * @param action Behavior of the handler; return `handler_flow::stop_chain` to end the chain.
 */
template <typename Predicate, typename Action>
conditional_handler<meta::decay_t<Predicate>, meta::decay_t<Action>, reject_policy::skip>
make_skippable_handler(Predicate&& predicate, Action&& action)
{
    return conditional_handler<meta::decay_t<Predicate>, meta::decay_t<Action>, reject_policy::skip>(
        CASTLE_FORWARD<Predicate>(predicate),
        CASTLE_FORWARD<Action>(action));
}

/**
 * @brief Creates a handler whose predicate always accepts, so it never rejects.
 * @param action Behavior of the handler; a `void` action lets the chain continue.
 */
template <typename Action>
conditional_handler<always_true, meta::decay_t<Action>>
make_default_handler(Action&& action)
{
    return conditional_handler<always_true, meta::decay_t<Action>>(
        always_true{},
        CASTLE_FORWARD<Action>(action));
}

/**
 * @brief Chain whose handler list is fixed by its type.
 *
 * @tparam Handlers Ordered `conditional_handler` types (at least one). The maximum number of nodes
 *         is exactly `sizeof...(Handlers)`.
 *
 * The chain owns every handler by value. It is callable (`operator()`), so a whole chain can serve
 * as the action of a handler in another chain or as a slot of a `castle::sigslot::signal`.
 */
template <typename... Handlers>
class chain_of_responsibility
{
    static_assert(sizeof...(Handlers) > 0U, "chain_of_responsibility requires at least one handler");

    template <typename... Ts>
    struct is_self_copy : meta::false_type
    {
    };

    template <typename Only>
    struct is_self_copy<Only> : meta::is_same<meta::decay_t<Only>, chain_of_responsibility>
    {
    };

public:
    /** @brief Number of handlers compiled into this chain. */
    static CASTLE_CONSTEXPR size_type handler_count = sizeof...(Handlers);

    chain_of_responsibility() CASTLE_DEFAULT;

    /**
     * @brief Constructs the chain by forwarding one object per handler type.
     * @param handlers Handler objects, in the order of `Handlers...`.
     */
    template <typename... HandlerArgs,
              typename = meta::enable_if_t<
                  sizeof...(HandlerArgs) == sizeof...(Handlers) &&
                  !is_self_copy<HandlerArgs...>::value>>
    explicit chain_of_responsibility(HandlerArgs&&... handlers)
        : handlers_(CASTLE_FORWARD<HandlerArgs>(handlers)...)
    {
    }

    /**
     * @brief Runs the handlers in declaration order until one ends the chain.
     * @param args Arguments passed, as lvalues, to every predicate and action.
     * @return `stopped` when an action returned `stop_chain`; `rejected` when a predicate was false on
     *         a `reject_policy::stop` handler; `completed` otherwise.
     * @note Complexity is O(H) with H = `handler_count`, a compile-time constant. Each handler
     *       evaluates at most one predicate and one action.
     */
    template <typename... Args>
    chain_result process(Args&&... args)
    {
        return process_impl(sequence::make_index_sequence_t<handler_count>{}, args...);
    }

    /**
     * @brief Callable form of `process()` so the chain can be nested or connected to a signal.
     * @return `continue_chain` when this chain completed; `stop_chain` when it stopped or rejected.
     */
    template <typename... Args>
    handler_flow operator()(Args&&... args)
    {
        return process(args...) == chain_result::completed ? handler_flow::continue_chain
                                                           : handler_flow::stop_chain;
    }

    /**
     * @brief Returns a handler by compile-time index.
     * @tparam Index Zero-based handler index; must be below `handler_count`.
     */
    template <size_type Index>
    tuple_element_t<Index, tuple<Handlers...>>& handler_at() CASTLE_NOEXCEPT
    {
        static_assert(Index < handler_count, "Handler index out of range");
        return castle::get<Index>(handlers_);
    }

    /** @brief Returns the compile-time number of handlers. */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return handler_count;
    }

private:
    template <size_type... Indices, typename... Args>
    chain_result process_impl(sequence::index_sequence<Indices...>, Args&... args)
    {
        chain_result result = chain_result::completed;

        // Once a handler ends the chain, later handlers are not evaluated at all.
        static_cast<void>((... , (result == chain_result::completed
                                      ? (result = castle::get<Indices>(handlers_).process(args...), true)
                                      : false)));

        return result;
    }

    tuple<Handlers...> handlers_{};
};

/**
 * @brief Creates a chain while deducing every handler type.
 * @param handlers Handlers in processing order.
 */
template <typename... Handlers>
chain_of_responsibility<meta::decay_t<Handlers>...> make_chain(Handlers&&... handlers)
{
    return chain_of_responsibility<meta::decay_t<Handlers>...>(CASTLE_FORWARD<Handlers>(handlers)...);
}

/**
 * @brief Forward declaration of the runtime-registered chain with compile-time capacity.
 *
 * @tparam MaxHandlers Maximum number of handlers; fixed at compile time.
 * @tparam Signature `void(Args...)`: the arguments given to `process()`.
 * @tparam StorageSize Inline bytes reserved per handler, for its predicate and action together.
 * @tparam StorageAlignment Alignment of that inline storage.
 */
template <size_type MaxHandlers,
          typename Signature,
          size_type StorageSize = castle::inplace_storage_reserved,
          size_type StorageAlignment = castle::inplace_alignment_default>
class bounded_chain_of_responsibility;

/**
 * @brief Chain whose handlers are registered at runtime into `MaxHandlers` fixed slots.
 *
 * Each handler is a `conditional_handler` (predicate plus action) stored in one
 * `castle::callbacks::function<chain_result(Args...)>` slot, so handlers of unrelated callable types
 * share one chain without heap allocation, and predicate and action semantics are exactly those of
 * the static chain. Handlers run in registration order. Registration is rejected while a `process()`
 * call is active.
 *
 * @note No internal synchronization; use one chain from one execution context or protect it.
 */
template <size_type MaxHandlers,
          typename... Args,
          size_type StorageSize,
          size_type StorageAlignment>
class bounded_chain_of_responsibility<MaxHandlers, void(Args...), StorageSize, StorageAlignment>
{
    static_assert(MaxHandlers > 0U, "bounded_chain_of_responsibility requires a positive capacity");
    static_assert(!meta::disjunction<meta::is_rvalue_reference<Args>...>::value,
                  "Chain arguments are shared by all handlers; rvalue reference parameters are not allowed");

public:
    /** @brief Type-erased storage of one handler. */
    using handler_type = callbacks::function<chain_result(Args...), StorageSize, StorageAlignment>;
    /** @brief Compact storage type for the handler count. */
    using index_type = meta::smallest_uint_t<MaxHandlers>;

    bounded_chain_of_responsibility() CASTLE_DEFAULT;

    /**
     * @brief Appends a handler that ends the chain (`rejected`) when its predicate is false.
     * @param predicate Callable invocable as `bool(Args&...)`.
     * @param action Callable invocable as `void(Args&...)` (implies `continue_chain`) or
     *        `handler_flow(Args&...)`.
     * @return `status::ok`; `status::full` when all slots are used; `status::invalid_config` while
     *         the chain is processing; `status::invalid_callback` when the predicate or action is a
     *         null function pointer or an empty `castle::callbacks::function`.
     * @warning Both callables must be copy-constructible and together fit `StorageSize`.
     */
    template <typename Predicate, typename Action>
    status add_handler(Predicate&& predicate, Action&& action)
    {
        return add_impl<reject_policy::stop>(CASTLE_FORWARD<Predicate>(predicate),
                                             CASTLE_FORWARD<Action>(action));
    }

    /**
     * @brief Appends a handler that is skipped when its predicate is false.
     * @param predicate Callable invocable as `bool(Args&...)`.
     * @param action Behavior of the handler; see `add_handler()`.
     * @return Same results as `add_handler()`.
     */
    template <typename Predicate, typename Action>
    status add_skip_handler(Predicate&& predicate, Action&& action)
    {
        return add_impl<reject_policy::skip>(CASTLE_FORWARD<Predicate>(predicate),
                                             CASTLE_FORWARD<Action>(action));
    }

    /**
     * @brief Appends a handler whose predicate always accepts, so it never rejects.
     * @param action Behavior of the handler; see `add_handler()`.
     * @return Same results as `add_handler()`.
     */
    template <typename Action>
    status add_default_handler(Action&& action)
    {
        return add_handler(always_true{}, CASTLE_FORWARD<Action>(action));
    }

    /**
     * @brief Runs the registered handlers in order until one ends the chain.
     * @param args Arguments handed to every predicate and action.
     * @return `stopped` when an action returned `stop_chain`; `rejected` when a predicate was false on
     *         a `reject_policy::stop` handler; `completed` otherwise, including when the chain is empty.
     * @note At most `size()` predicate and `size()` action calls; the bound is `MaxHandlers`.
     */
    chain_result process(Args... args)
    {
        chain_result result = chain_result::completed;

        ++depth_;
        for (size_type i = 0U; i < size_ && result == chain_result::completed; ++i)
        {
            result = handlers_[i](args...);
        }
        --depth_;

        return result;
    }

    /**
     * @brief Callable form of `process()` so the chain can be nested or connected to a signal.
     * @return `continue_chain` when this chain completed; `stop_chain` when it stopped or rejected.
     */
    handler_flow operator()(Args... args)
    {
        return process(args...) == chain_result::completed ? handler_flow::continue_chain
                                                           : handler_flow::stop_chain;
    }

    /**
     * @brief Removes every handler.
     * @return `status::ok`, or `status::invalid_config` while the chain is processing.
     */
    status clear() CASTLE_NOEXCEPT
    {
        if (depth_ != 0U)
        {
            return status::invalid_config;
        }

        for (size_type i = 0U; i < size_; ++i)
        {
            handlers_[i] = nullptr;
        }
        size_ = 0U;
        return status::ok;
    }

    /** @brief Returns the number of registered handlers. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_;
    }

    /** @brief Returns whether no handler is registered. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == 0U;
    }

    /** @brief Returns whether `process()` is currently running (called from inside a handler). */
    bool is_processing() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return depth_ != 0U;
    }

    /** @brief Returns the compile-time maximum number of handlers. */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return MaxHandlers;
    }

private:
    // Adapts conditional_handler::process(Args&...) to the handler_type call signature.
    template <typename Body>
    struct invoker
    {
        chain_result operator()(Args... args)
        {
            return body.process(args...);
        }

        Body body;
    };

    template <reject_policy OnReject, typename Predicate, typename Action>
    status add_impl(Predicate&& predicate, Action&& action)
    {
        using body_type = conditional_handler<meta::decay_t<Predicate>, meta::decay_t<Action>, OnReject>;
        static_assert(meta::is_copy_constructible<meta::decay_t<Predicate>>::value &&
                          meta::is_copy_constructible<meta::decay_t<Action>>::value,
                      "Chain callables must be copy-constructible");

        if (depth_ != 0U)
        {
            return status::invalid_config;
        }

        if (size_ >= MaxHandlers)
        {
            return status::full;
        }

        // Emptiness must be checked before wrapping: the wrapper itself is never empty.
        if (is_empty_callable(predicate) || is_empty_callable(action))
        {
            return status::invalid_callback;
        }

        handlers_[size_] = handler_type{invoker<body_type>{
            body_type(CASTLE_FORWARD<Predicate>(predicate), CASTLE_FORWARD<Action>(action))}};
        ++size_;
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

    dsa::array<handler_type, MaxHandlers> handlers_{};
    index_type size_ = 0U;
    size_type depth_ = 0U;
};

} // namespace patterns
} // namespace castle

#endif // CASTLE_PATTERNS_CHAIN_OF_RESPONSIBILITY_HPP
