// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file composite.hpp
 * @brief Compile-time heterogeneous Composite with generic, status-aware visitor traversal.
 *
 * `composite<Components...>` groups a fixed set of heterogeneous objects without
 * requiring a shared base class. Components are stored in CASTLE's STL-free
 * `castle::tuple`; each component may be owned by value or held by reference.
 * A visitor supplied by the caller defines the operation, so this header does
 * not impose a lifecycle, rendering, serialization, or other domain-specific
 * interface on leaf components.
 *
 * Nested composites form a tree that is treated like a single object:
 * - `for_each()` visits every leaf (nested groups are expanded),
 * - `for_each_child()` visits only the immediate children,
 * - `for_each_node()` visits every descendant, including nested groups.
 *
 * Every traversal accepts an optional order tag. `forward_order` (default) is
 * depth-first, left-to-right; `reverse_order` is the exact mirror, which is the
 * natural teardown order for groups built in startup order.
 *
 * Visitors may return `void` or `castle::status`. Traversals visit every node
 * and return the first failure (or `status::ok`). `try_for_each()` instead stops
 * at the first failing leaf. All dispatch is static and allocation-free.
 *
 * @code
 * #include "castle/patterns/composite.hpp"
 *
 * namespace dp = castle::patterns;
 *
 * struct sensor { castle::status start() noexcept; castle::status stop() noexcept; };
 * struct logger { castle::status start() noexcept; castle::status stop() noexcept; };
 *
 * sensor temperature;
 * logger diagnostics;
 * auto services = dp::tie_composite(temperature, diagnostics);
 *
 * // Fail-fast startup in declaration order.
 * castle::status started = services.try_for_each([](auto& c) { return c.start(); });
 *
 * // Stop everything in reverse order, reporting the first failure.
 * castle::status stopped = services.for_each(dp::reverse_order, [](auto& c) { return c.stop(); });
 * @endcode
 *
 * @note `make_composite()` stores decayed component values. `tie_composite()`
 *       stores references; referenced objects must outlive the composite.
 * @note The number and types of children are fixed by the template arguments.
 *       Runtime insertion/removal is intentionally not provided; use
 *       `castle::callbacks::delegate_registry` or `callback_dispatcher` for a
 *       runtime list of callbacks sharing one signature.
 */
#ifndef CASTLE_PATTERNS_COMPOSITE_HPP
#define CASTLE_PATTERNS_COMPOSITE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/integral_sequence.hpp"
#include "castle/utility/tuple.hpp"

#include <stdint.h>

namespace castle
{
namespace patterns
{

template <typename... Components>
class composite;

/** @brief Tag type selecting depth-first, left-to-right traversal. */
struct forward_order_t
{
    explicit CASTLE_CONSTEXPR forward_order_t() CASTLE_NOEXCEPT {}
};

/** @brief Tag type selecting the mirrored (depth-first, right-to-left) traversal. */
struct reverse_order_t
{
    explicit CASTLE_CONSTEXPR reverse_order_t() CASTLE_NOEXCEPT {}
};

/** @brief Selects declaration-order traversal (the default). */
CASTLE_INLINE CASTLE_CONSTEXPR forward_order_t forward_order{};

/** @brief Selects reverse-declaration-order traversal, e.g. for teardown. */
CASTLE_INLINE CASTLE_CONSTEXPR reverse_order_t reverse_order{};

namespace composite_detail
{

template <typename T>
struct is_composite_impl : meta::false_type
{
};

template <typename... Components>
struct is_composite_impl<composite<Components...> > : meta::true_type
{
};

} // namespace composite_detail

/**
 * @brief Reports whether `T` is a `composite` specialization, ignoring cv- and reference qualifiers.
 *
 * Useful inside `for_each_node()` / `for_each_child()` visitors to tell groups from leaves.
 */
template <typename T>
struct is_composite : composite_detail::is_composite_impl<meta::decay_t<T> >
{
};

namespace composite_detail
{

enum class scope : uint8_t
{
    leaves,
    children,
    nodes
};

template <typename T>
struct is_order_tag : meta::false_type
{
};

template <>
struct is_order_tag<forward_order_t> : meta::true_type
{
};

template <>
struct is_order_tag<reverse_order_t> : meta::true_type
{
};

template <typename T, typename... Args>
struct is_single_argument_same : meta::false_type
{
};

template <typename T, typename Arg>
struct is_single_argument_same<T, Arg> : meta::is_same<meta::decay_t<Arg>, T>
{
};

template <typename T, bool IsComposite = is_composite<T>::value>
struct leaf_count : meta::integral_constant<size_type, 1U>
{
};

template <typename T>
struct leaf_count<T, true>
    : meta::integral_constant<size_type, meta::decay_t<T>::leaf_count_value>
{
};

// Yields `void` for non-invocable pairs so only the intended static_assert fires.
template <typename Visitor, typename Node, bool = meta::is_invocable<Visitor&, Node&>::value>
struct visit_result
{
    using type = void;
};

template <typename Visitor, typename Node>
struct visit_result<Visitor, Node, true>
{
    using type = meta::invoke_result_t<Visitor&, Node&>;
};

template <typename Visitor, typename Node>
status invoke_visitor(Visitor& visitor, Node& node)
{
    static_assert(meta::is_invocable<Visitor&, Node&>::value,
                  "composite visitor must be invocable with every visited node");

    using result_type = typename visit_result<Visitor, Node>::type;
    static_assert(meta::is_void<result_type>::value ||
                      meta::is_same<meta::decay_t<result_type>, status>::value,
                  "composite visitor must return void or castle::status");

    if constexpr (meta::is_void<result_type>::value)
    {
        visitor(node);
        return status::ok;
    }
    else
    {
        return visitor(node);
    }
}

/** Keeps the first failure in `result`; returns whether `result` is still successful. */
CASTLE_CONSTEXPR bool record_first_failure(status& result, status value) CASTLE_NOEXCEPT
{
    if (succeeded(result) && !succeeded(value))
    {
        result = value;
    }
    return succeeded(result);
}

} // namespace composite_detail

/**
 * @brief A fixed heterogeneous group that can be traversed like its leaf components.
 *
 * @tparam Components Component types stored in this group. Value types are owned;
 *         lvalue-reference types are non-owning references. May be empty.
 *
 * `composite` is a value type when all value components are value types. Nested
 * composites are traversed recursively, making a tree of groups usable through
 * one generic visitor.
 *
 * Visitors are invoked as lvalues and must accept every node type they receive.
 * They must return `void` (never fails) or `castle::status`.
 */
template <typename... Components>
class composite
{
public:
    using size_type = castle::size_type;
    using storage_type = castle::tuple<Components...>;

    static CASTLE_CONSTEXPR size_type component_count = sizeof...(Components);
    static CASTLE_CONSTEXPR size_type leaf_count_value =
        (composite_detail::leaf_count<Components>::value + ... + 0U);

    /** @brief Default-constructs an empty or default-constructible composition. */
    composite() CASTLE_DEFAULT;

    /**
     * @brief Constructs the group from its component values or references.
     * @tparam Args Constructor argument types.
     * @param components Values/references used to initialize the corresponding
     *        `Components...` entries.
     *
     * The argument count must match the number of template components. A
     * component is copied or moved according to its declared storage type.
     */
    template <
        typename... Args,
        typename meta::enable_if<
            (sizeof...(Components) > 0U) &&
                (sizeof...(Args) == sizeof...(Components)) &&
                !composite_detail::is_single_argument_same<composite, Args...>::value,
            int>::type = 0>
    explicit composite(Args&&... components)
        : components_(CASTLE_FORWARD<Args>(components)...)
    {
    }

    composite(CASTLE_CONST composite&) CASTLE_DEFAULT;
    composite(composite&&) CASTLE_DEFAULT;
    composite& operator=(CASTLE_CONST composite&) CASTLE_DEFAULT;
    composite& operator=(composite&&) CASTLE_DEFAULT;
    ~composite() CASTLE_DEFAULT;

    /** @brief Returns the number of immediate children. */
    static CASTLE_CONSTEXPR size_type size() CASTLE_NOEXCEPT
    {
        return component_count;
    }

    /** @brief Returns the total number of leaf components below this group. */
    static CASTLE_CONSTEXPR size_type leaf_size() CASTLE_NOEXCEPT
    {
        return leaf_count_value;
    }

    /** @brief Reports whether this group has no immediate children. */
    static CASTLE_CONSTEXPR bool empty() CASTLE_NOEXCEPT
    {
        return component_count == 0U;
    }

    /**
     * @brief Returns an immediate child by compile-time index.
     * @tparam Index Zero-based child index.
     * @return Reference to the selected component.
     */
    template <size_type Index>
    decltype(auto) get() CASTLE_NOEXCEPT
    {
        static_assert(Index < component_count, "composite child index is out of range");
        return castle::get<Index>(components_);
    }

    /** @brief Returns a const immediate child by compile-time index. */
    template <size_type Index>
    decltype(auto) get() CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(Index < component_count, "composite child index is out of range");
        return castle::get<Index>(components_);
    }

    /**
     * @brief Applies a visitor to every leaf component in the composition tree.
     * @param visitor Operation to execute for each leaf. Returns `void` or `castle::status`.
     * @return The first failure reported by the visitor, or `status::ok`.
     *
     * Nested composites are expanded recursively and are not passed to the
     * visitor. Every leaf is visited even after a failure; use `try_for_each()`
     * to stop at the first failure.
     *
     * @note Complexity: O(number of nodes in the composition tree), with no
     *       dynamic or container storage. Runtime call-stack depth is
     *       proportional to the composition nesting depth.
     */
    template <typename Visitor>
    status for_each(Visitor&& visitor)
    {
        return for_each(forward_order, visitor);
    }

    /** @brief Const overload of `for_each()`. */
    template <typename Visitor>
    status for_each(Visitor&& visitor) CASTLE_CONST
    {
        return for_each(forward_order, visitor);
    }

    /**
     * @brief Applies a visitor to every leaf in the selected order.
     * @param order `forward_order` (depth-first, left-to-right) or `reverse_order` (mirrored).
     * @param visitor Operation to execute for each leaf.
     * @return The first failure reported by the visitor, or `status::ok`.
     */
    template <typename Order, typename Visitor>
    status for_each(Order order, Visitor&& visitor)
    {
        return traverse<composite_detail::scope::leaves, false>(*this, order, visitor);
    }

    /** @brief Const overload of `for_each(order, visitor)`. */
    template <typename Order, typename Visitor>
    status for_each(Order order, Visitor&& visitor) CASTLE_CONST
    {
        return traverse<composite_detail::scope::leaves, false>(*this, order, visitor);
    }

    /**
     * @brief Applies a visitor to leaves until one fails.
     * @param visitor Operation to execute for each leaf. Returns `void` or `castle::status`.
     * @return The failing status, or `status::ok` when every leaf succeeded.
     *
     * Leaves after the failing one, including those in enclosing groups, are not visited.
     */
    template <typename Visitor>
    CASTLE_NODISCARD status try_for_each(Visitor&& visitor)
    {
        return try_for_each(forward_order, visitor);
    }

    /** @brief Const overload of `try_for_each()`. */
    template <typename Visitor>
    CASTLE_NODISCARD status try_for_each(Visitor&& visitor) CASTLE_CONST
    {
        return try_for_each(forward_order, visitor);
    }

    /** @brief Fail-fast leaf traversal in the selected order. */
    template <typename Order, typename Visitor>
    CASTLE_NODISCARD status try_for_each(Order order, Visitor&& visitor)
    {
        return traverse<composite_detail::scope::leaves, true>(*this, order, visitor);
    }

    /** @brief Const overload of `try_for_each(order, visitor)`. */
    template <typename Order, typename Visitor>
    CASTLE_NODISCARD status try_for_each(Order order, Visitor&& visitor) CASTLE_CONST
    {
        return traverse<composite_detail::scope::leaves, true>(*this, order, visitor);
    }

    /**
     * @brief Applies a visitor to the immediate children only.
     * @param visitor Operation to execute for each child. Returns `void` or `castle::status`.
     * @return The first failure reported by the visitor, or `status::ok`.
     *
     * Unlike `for_each()`, a nested composite is passed as one child and is not
     * expanded.
     */
    template <typename Visitor>
    status for_each_child(Visitor&& visitor)
    {
        return for_each_child(forward_order, visitor);
    }

    /** @brief Const overload of `for_each_child()`. */
    template <typename Visitor>
    status for_each_child(Visitor&& visitor) CASTLE_CONST
    {
        return for_each_child(forward_order, visitor);
    }

    /** @brief Applies a visitor to the immediate children in the selected order. */
    template <typename Order, typename Visitor>
    status for_each_child(Order order, Visitor&& visitor)
    {
        return traverse<composite_detail::scope::children, false>(*this, order, visitor);
    }

    /** @brief Const overload of `for_each_child(order, visitor)`. */
    template <typename Order, typename Visitor>
    status for_each_child(Order order, Visitor&& visitor) CASTLE_CONST
    {
        return traverse<composite_detail::scope::children, false>(*this, order, visitor);
    }

    /**
     * @brief Applies a visitor to every descendant node, including nested composites.
     * @param visitor Operation to execute for each node. Returns `void` or `castle::status`.
     * @return The first failure reported by the visitor, or `status::ok`.
     *
     * The root object itself is not passed. In `forward_order` (pre-order) a
     * group is visited before its descendants; in `reverse_order` the sequence
     * is mirrored, so descendants are visited before their group.
     */
    template <typename Visitor>
    status for_each_node(Visitor&& visitor)
    {
        return for_each_node(forward_order, visitor);
    }

    /** @brief Const overload of `for_each_node()`. */
    template <typename Visitor>
    status for_each_node(Visitor&& visitor) CASTLE_CONST
    {
        return for_each_node(forward_order, visitor);
    }

    /** @brief Applies a visitor to every descendant node in the selected order. */
    template <typename Order, typename Visitor>
    status for_each_node(Order order, Visitor&& visitor)
    {
        return traverse<composite_detail::scope::nodes, false>(*this, order, visitor);
    }

    /** @brief Const overload of `for_each_node(order, visitor)`. */
    template <typename Order, typename Visitor>
    status for_each_node(Order order, Visitor&& visitor) CASTLE_CONST
    {
        return traverse<composite_detail::scope::nodes, false>(*this, order, visitor);
    }

private:
    template <typename...>
    friend class composite;

    template <composite_detail::scope Scope, bool StopOnFailure, typename Self, typename Order, typename Visitor>
    static status traverse(Self& self, Order, Visitor& visitor)
    {
        static_assert(composite_detail::is_order_tag<Order>::value,
                      "composite traversal order must be forward_order or reverse_order");

        static_assert(!StopOnFailure || (Scope != composite_detail::scope::nodes),
                      "fail-fast traversal is provided for leaves only");

        return traverse_children<Scope, meta::is_same<Order, reverse_order_t>::value, StopOnFailure>(self, visitor);
    }

    template <composite_detail::scope Scope, bool Reverse, bool StopOnFailure, typename Self, typename Visitor>
    static status traverse_children(Self& self, Visitor& visitor)
    {
        return traverse_impl<Scope, Reverse, StopOnFailure>(self, visitor, sequence::index_sequence_for<Components...>{});
    }

    template <bool Reverse>
    static CASTLE_CONSTEXPR size_type child_index(size_type index) CASTLE_NOEXCEPT
    {
        return Reverse ? (component_count - 1U - index) : index;
    }

    template <composite_detail::scope Scope, bool Reverse, bool StopOnFailure,
              typename Self, typename Visitor, size_type... Indices>
    static status traverse_impl(CASTLE_UNUSED Self& self,
                                CASTLE_UNUSED Visitor& visitor,
                                sequence::index_sequence<Indices...>)
    {
        status result = status::ok;
        if constexpr (StopOnFailure)
        {
            // `&&` short-circuits, so later children are skipped after a failure.
            static_cast<void>((visit_child<Scope, Reverse, StopOnFailure>(
                                   castle::get<child_index<Reverse>(Indices)>(self.components_), visitor, result) &&
                               ...));
        }
        else
        {
            (static_cast<void>(visit_child<Scope, Reverse, StopOnFailure>(
                 castle::get<child_index<Reverse>(Indices)>(self.components_), visitor, result)),
             ...);
        }
        return result;
    }

    // Returns whether traversal may continue (always true unless StopOnFailure and a failure occurred).
    template <composite_detail::scope Scope, bool Reverse, bool StopOnFailure, typename Node, typename Visitor>
    static bool visit_child(Node& node, Visitor& visitor, status& result)
    {
        using node_type = meta::decay_t<Node>;

        if constexpr (!is_composite<node_type>::value || (Scope == composite_detail::scope::children))
        {
            return composite_detail::record_first_failure(result, composite_detail::invoke_visitor(visitor, node)) ||
                   !StopOnFailure;
        }
        else if constexpr (Scope == composite_detail::scope::leaves)
        {
            return composite_detail::record_first_failure(
                       result, node_type::template traverse_children<Scope, Reverse, StopOnFailure>(node, visitor)) ||
                   !StopOnFailure;
        }
        else
        {
            // Node scope: pre-order forward; the mirrored sequence (post-order) in reverse.
            if constexpr (!Reverse)
            {
                composite_detail::record_first_failure(result, composite_detail::invoke_visitor(visitor, node));
            }
            composite_detail::record_first_failure(
                result, node_type::template traverse_children<Scope, Reverse, StopOnFailure>(node, visitor));
            if constexpr (Reverse)
            {
                composite_detail::record_first_failure(result, composite_detail::invoke_visitor(visitor, node));
            }
            return true;
        }
    }

    storage_type components_{};
};

/**
 * @brief Creates a composite that owns decayed copies or moves of its components.
 * @param components Values forwarded into the composite.
 * @return `composite<meta::decay_t<Components>...>`.
 */
template <typename... Components>
composite<meta::decay_t<Components>...>
make_composite(Components&&... components)
{
    return composite<meta::decay_t<Components>...>(CASTLE_FORWARD<Components>(components)...);
}

/**
 * @brief Creates a non-owning composite that refers to existing lvalue components.
 * @param components Existing objects to reference.
 * @return `composite<Components&...>`.
 * @warning Every referenced object must outlive the returned composite and any
 *          composite that stores it by value.
 */
template <typename... Components>
composite<Components&...>
tie_composite(Components&... components)
{
    return composite<Components&...>(components...);
}

} // namespace patterns
} // namespace castle

#endif // CASTLE_PATTERNS_COMPOSITE_HPP
