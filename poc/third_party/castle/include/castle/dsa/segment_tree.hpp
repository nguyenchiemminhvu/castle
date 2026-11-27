// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file segment_tree.hpp
 * @brief Fixed-capacity segment trees: point update / range query, and lazy range update / range query.
 *
 * A segment tree keeps the aggregate of every aligned block of a sequence, so any interval is the
 * combination of O(log n) blocks. The aggregate is described by a monoid policy: a value type, an
 * associative `combine()`, and its neutral `identity()`. Sum, minimum, maximum, gcd, bitwise and/or,
 * matrix products, or a custom struct such as "best subarray sum" all fit. `combine()` need not be
 * commutative; operands are always combined left to right.
 *
 * - `segment_tree<Monoid, Capacity>`: point assignment, inclusive range query, and binary search
 *   on prefix aggregates (`max_right()`, `min_left()`). Queries are `const`.
 * - `lazy_segment_tree<Policy, Capacity>`: adds range updates (`apply(left, right, tag)`) such as
 *   "add 5 to [l, r]" or "set [l, r] to 0". Pending updates are stored per node as tags and pushed
 *   down only when a query or update passes through, keeping every operation O(log n).
 *
 * Both trees are iterative (no recursion), store a perfect binary tree over the next power of two of
 * the current size, and never allocate.
 *
 * @code
 * castle::dsa::segment_tree<castle::dsa::sum_monoid<int32_t>, 8U> sums;
 * const int32_t values[] = {1, 3, 5};
 * sums.build(values, 3U);
 * sums.set(1U, 2);                      // values = {1, 2, 5}
 * int32_t total = sums.query(0U, 2U);   // 8
 *
 * castle::dsa::lazy_segment_tree<castle::dsa::range_add_max<int32_t>, 8U> peaks;
 * peaks.assign(6U, 0);
 * peaks.apply(1U, 3U, 2);               // {0, 2, 2, 2, 0, 0}
 * int32_t highest = peaks.query(0U, 5U); // 2
 * @endcode
 *
 * @see https://cp-algorithms.com/data_structures/segment_tree.html
 * @see https://codeforces.com/blog/entry/18051
 */
#ifndef CASTLE_DSA_SEGMENT_TREE_HPP
#define CASTLE_DSA_SEGMENT_TREE_HPP

#include "castle/bit/bit_math.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/array.hpp"

namespace castle
{
namespace dsa
{

/**
 * @brief Addition monoid: `combine(a, b) = a + b`, `identity() = T()`.
 * @tparam T Arithmetic-like type whose default value is zero.
 * @note A segment tree monoid is any type providing `value_type`, a static `identity()`, and a
 * static associative `combine(lhs, rhs)` for which `identity()` is neutral.
 */
template <typename T>
struct sum_monoid
{
    using value_type = T;

    /** @brief Returns zero. */
    static CASTLE_CONSTEXPR T identity() CASTLE_NOEXCEPT
    {
        return T();
    }

    /** @brief Returns `lhs + rhs`. */
    static CASTLE_CONSTEXPR T combine(CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_NOEXCEPT
    {
        return lhs + rhs;
    }
};

/**
 * @brief Minimum monoid: `combine(a, b) = min(a, b)`, `identity() = numeric_limits<T>::max()`.
 * @tparam T Type with a `castle::numeric_limits` specialization.
 */
template <typename T>
struct min_monoid
{
    using value_type = T;

    /** @brief Returns the largest value of `T`. */
    static CASTLE_CONSTEXPR T identity() CASTLE_NOEXCEPT
    {
        return numeric_limits<T>::max();
    }

    /** @brief Returns the smaller operand; `lhs` on ties. */
    static CASTLE_CONSTEXPR T combine(CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_NOEXCEPT
    {
        return (rhs < lhs) ? rhs : lhs;
    }
};

/**
 * @brief Maximum monoid: `combine(a, b) = max(a, b)`, `identity() = numeric_limits<T>::lowest()`.
 * @tparam T Type with a `castle::numeric_limits` specialization.
 */
template <typename T>
struct max_monoid
{
    using value_type = T;

    /** @brief Returns the lowest value of `T`. */
    static CASTLE_CONSTEXPR T identity() CASTLE_NOEXCEPT
    {
        return numeric_limits<T>::lowest();
    }

    /** @brief Returns the larger operand; `lhs` on ties. */
    static CASTLE_CONSTEXPR T combine(CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_NOEXCEPT
    {
        return (lhs < rhs) ? rhs : lhs;
    }
};

/**
 * @brief Range-add policy over sums: adding `tag` to `length` elements adds `tag * length`.
 * @tparam T Arithmetic type.
 * @note A lazy policy is a monoid plus `tag_type`, a static `tag_identity()` (the no-op update),
 * `apply(tag, value, length)` that updates the aggregate of `length` elements, and
 * `compose(newer, older)` that merges two pending updates into one equal to `older` then `newer`.
 */
template <typename T>
struct range_add_sum : sum_monoid<T>
{
    using tag_type = T;

    /** @brief Returns zero: adding nothing. */
    static CASTLE_CONSTEXPR T tag_identity() CASTLE_NOEXCEPT
    {
        return T();
    }

    /** @brief Returns `value + tag * length`. */
    static CASTLE_CONSTEXPR T apply(CASTLE_CONST T& tag, CASTLE_CONST T& value, size_type length) CASTLE_NOEXCEPT
    {
        return value + tag * static_cast<T>(length);
    }

    /** @brief Returns `newer + older`. */
    static CASTLE_CONSTEXPR T compose(CASTLE_CONST T& newer, CASTLE_CONST T& older) CASTLE_NOEXCEPT
    {
        return newer + older;
    }
};

/**
 * @brief Range-add policy over minimums: adding `tag` to every element adds `tag` to the minimum.
 * @tparam T Arithmetic type with a `castle::numeric_limits` specialization.
 */
template <typename T>
struct range_add_min : min_monoid<T>
{
    using tag_type = T;

    /** @brief Returns zero: adding nothing. */
    static CASTLE_CONSTEXPR T tag_identity() CASTLE_NOEXCEPT
    {
        return T();
    }

    /** @brief Returns `value + tag`. */
    static CASTLE_CONSTEXPR T apply(CASTLE_CONST T& tag, CASTLE_CONST T& value, size_type) CASTLE_NOEXCEPT
    {
        return value + tag;
    }

    /** @brief Returns `newer + older`. */
    static CASTLE_CONSTEXPR T compose(CASTLE_CONST T& newer, CASTLE_CONST T& older) CASTLE_NOEXCEPT
    {
        return newer + older;
    }
};

/**
 * @brief Range-add policy over maximums: adding `tag` to every element adds `tag` to the maximum.
 * @tparam T Arithmetic type with a `castle::numeric_limits` specialization.
 */
template <typename T>
struct range_add_max : max_monoid<T>
{
    using tag_type = T;

    /** @brief Returns zero: adding nothing. */
    static CASTLE_CONSTEXPR T tag_identity() CASTLE_NOEXCEPT
    {
        return T();
    }

    /** @brief Returns `value + tag`. */
    static CASTLE_CONSTEXPR T apply(CASTLE_CONST T& tag, CASTLE_CONST T& value, size_type) CASTLE_NOEXCEPT
    {
        return value + tag;
    }

    /** @brief Returns `newer + older`. */
    static CASTLE_CONSTEXPR T compose(CASTLE_CONST T& newer, CASTLE_CONST T& older) CASTLE_NOEXCEPT
    {
        return newer + older;
    }
};

/**
 * @brief Pending "assign `value`" update used by the `range_assign_*` policies.
 * @tparam T Element type.
 * @note Converts implicitly from `T`, so `tree.apply(left, right, 7)` assigns 7. A default-constructed
 * tag is inactive (no update).
 */
template <typename T>
struct assign_tag
{
    /** @brief Constructs the inactive (no-op) tag. */
    CASTLE_CONSTEXPR assign_tag() CASTLE_NOEXCEPT
        : value(), active(false)
    {
    }

    /** @brief Constructs an active tag that assigns `assigned`. */
    CASTLE_CONSTEXPR assign_tag(CASTLE_CONST T& assigned) CASTLE_NOEXCEPT // NOLINT(google-explicit-constructor)
        : value(assigned), active(true)
    {
    }

    T value;
    bool active;
};

/**
 * @brief Range-assign policy over sums: assigning `v` to `length` elements makes the sum `v * length`.
 * @tparam T Arithmetic type.
 */
template <typename T>
struct range_assign_sum : sum_monoid<T>
{
    using tag_type = assign_tag<T>;

    /** @brief Returns the inactive tag. */
    static CASTLE_CONSTEXPR tag_type tag_identity() CASTLE_NOEXCEPT
    {
        return tag_type();
    }

    /** @brief Returns `tag.value * length` for an active tag, else `value`. */
    static CASTLE_CONSTEXPR T apply(CASTLE_CONST tag_type& tag, CASTLE_CONST T& value, size_type length) CASTLE_NOEXCEPT
    {
        return tag.active ? tag.value * static_cast<T>(length) : value;
    }

    /** @brief The newer active assignment wins. */
    static CASTLE_CONSTEXPR tag_type compose(CASTLE_CONST tag_type& newer, CASTLE_CONST tag_type& older) CASTLE_NOEXCEPT
    {
        return newer.active ? newer : older;
    }
};

/**
 * @brief Range-assign policy over minimums: assigning `v` to every element makes the minimum `v`.
 * @tparam T Type with a `castle::numeric_limits` specialization.
 */
template <typename T>
struct range_assign_min : min_monoid<T>
{
    using tag_type = assign_tag<T>;

    /** @brief Returns the inactive tag. */
    static CASTLE_CONSTEXPR tag_type tag_identity() CASTLE_NOEXCEPT
    {
        return tag_type();
    }

    /** @brief Returns `tag.value` for an active tag, else `value`. */
    static CASTLE_CONSTEXPR T apply(CASTLE_CONST tag_type& tag, CASTLE_CONST T& value, size_type) CASTLE_NOEXCEPT
    {
        return tag.active ? tag.value : value;
    }

    /** @brief The newer active assignment wins. */
    static CASTLE_CONSTEXPR tag_type compose(CASTLE_CONST tag_type& newer, CASTLE_CONST tag_type& older) CASTLE_NOEXCEPT
    {
        return newer.active ? newer : older;
    }
};

/**
 * @brief Range-assign policy over maximums: assigning `v` to every element makes the maximum `v`.
 * @tparam T Type with a `castle::numeric_limits` specialization.
 */
template <typename T>
struct range_assign_max : max_monoid<T>
{
    using tag_type = assign_tag<T>;

    /** @brief Returns the inactive tag. */
    static CASTLE_CONSTEXPR tag_type tag_identity() CASTLE_NOEXCEPT
    {
        return tag_type();
    }

    /** @brief Returns `tag.value` for an active tag, else `value`. */
    static CASTLE_CONSTEXPR T apply(CASTLE_CONST tag_type& tag, CASTLE_CONST T& value, size_type) CASTLE_NOEXCEPT
    {
        return tag.active ? tag.value : value;
    }

    /** @brief The newer active assignment wins. */
    static CASTLE_CONSTEXPR tag_type compose(CASTLE_CONST tag_type& newer, CASTLE_CONST tag_type& older) CASTLE_NOEXCEPT
    {
        return newer.active ? newer : older;
    }
};

namespace detail
{

/** @brief Returns `true` when `value` is a power of two (or zero). */
CASTLE_CONSTEXPR bool segment_tree_is_power_of_two(size_type value) CASTLE_NOEXCEPT
{
    return (value & (value - 1U)) == 0U;
}

} // namespace detail

/**
 * @brief Segment tree with point assignment and inclusive range queries.
 * @tparam Monoid Aggregate policy, for example `sum_monoid<int32_t>`, `min_monoid<uint16_t>`, or a
 * user-defined monoid.
 * @tparam Capacity Maximum number of elements. Must be greater than zero.
 * @note Storage is `2 * next_power_of_two(Capacity)` values. With `size()` elements, operations run
 * over a perfect tree of `next_power_of_two(size())` leaves; unused leaves hold `identity()`.
 * @warning Out-of-range arguments are rejected in every build: mutators return
 * `status::out_of_range`, `get()`/`query()` return `identity()`, and the binary searches return `npos`.
 */
template <typename Monoid, size_type Capacity>
class segment_tree
{
    static_assert(Capacity > 0U, "dsa::segment_tree requires a positive capacity");
    static_assert(Capacity <= (static_cast<castle::size_type>(-1) / 4U), "dsa::segment_tree capacity is too large");

public:
    using monoid_type = Monoid;
    using value_type = typename Monoid::value_type;
    using size_type = castle::size_type;

    /** @brief Compile-time maximum number of elements. */
    static CASTLE_CONSTEXPR size_type static_capacity = Capacity;
    /** @brief Leaf slots in the embedded storage, `next_power_of_two(Capacity)`. */
    static CASTLE_CONSTEXPR size_type static_leaf_capacity = bit::next_power_of_two_const<Capacity>::value;
    /** @brief Returned by `max_right()` / `min_left()` for an out-of-range bound. */
    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

    /** @brief Constructs an empty tree. Complexity: O(Capacity) for value-initializing storage. */
    segment_tree() CASTLE_NOEXCEPT
        : tree_{}, size_(0U), base_(1U)
    {
        tree_[1U] = Monoid::identity();
    }

    /**
     * @brief Replaces the contents with `count` values.
     * @param values Source values; may be null only when `count == 0`.
     * @param count Number of values.
     * @return `status::ok`, `status::out_of_range` when `count > Capacity`, or
     * `status::invalid_argument` for a null source. The tree is unchanged on failure.
     * @note Complexity: O(count).
     */
    status build(CASTLE_CONST value_type* values, size_type count) CASTLE_NOEXCEPT
    {
        if (count > Capacity)
        {
            return status::out_of_range;
        }
        if ((count > 0U) && (values == nullptr))
        {
            return status::invalid_argument;
        }
        resize(count);
        for (size_type i = 0U; i < count; ++i)
        {
            tree_[base_ + i] = values[i];
        }
        rebuild();
        return status::ok;
    }

    /**
     * @brief Replaces the contents with `count` copies of `value`.
     * @return `status::ok`, or `status::out_of_range` when `count > Capacity` (tree unchanged).
     * @note Complexity: O(count).
     */
    status assign(size_type count, CASTLE_CONST value_type& value = Monoid::identity()) CASTLE_NOEXCEPT
    {
        if (count > Capacity)
        {
            return status::out_of_range;
        }
        resize(count);
        for (size_type i = 0U; i < count; ++i)
        {
            tree_[base_ + i] = value;
        }
        rebuild();
        return status::ok;
    }

    /**
     * @brief Replaces the element at `index` and recomputes its ancestors.
     * @return `status::ok` or `status::out_of_range`.
     * @note Complexity: O(log size()).
     */
    status set(size_type index, CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return status::out_of_range;
        }
        size_type node = base_ + index;
        tree_[node] = value;
        while (node > 1U)
        {
            node >>= 1U;
            pull(node);
        }
        return status::ok;
    }

    /**
     * @brief Returns the element at `index`, or `identity()` when `index >= size()`.
     * @note Complexity: O(1).
     */
    value_type get(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (index < size_) ? tree_[base_ + index] : Monoid::identity();
    }

    /**
     * @brief Returns the aggregate of the inclusive interval `[left, right]`, combined left to right.
     * @return Aggregate, or `identity()` when `left > right` or `right >= size()`.
     * @note Complexity: O(log size()).
     */
    value_type query(size_type left, size_type right) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if ((left > right) || (right >= size_))
        {
            return Monoid::identity();
        }
        size_type first = base_ + left;
        size_type last = base_ + right + 1U;
        value_type left_result = Monoid::identity();
        value_type right_result = Monoid::identity();
        while (first < last)
        {
            if ((first & 1U) != 0U)
            {
                left_result = Monoid::combine(left_result, tree_[first]);
                ++first;
            }
            if ((last & 1U) != 0U)
            {
                --last;
                right_result = Monoid::combine(tree_[last], right_result);
            }
            first >>= 1U;
            last >>= 1U;
        }
        return Monoid::combine(left_result, right_result);
    }

    /** @brief Returns the aggregate of all elements (`identity()` when empty). Complexity: O(1). */
    value_type all() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_[1U]; // the root; identity() for an empty tree
    }

    /**
     * @brief Extends a half-open interval `[left, end)` as far right as `pred` allows.
     * @tparam Predicate Callable as `bool(const value_type&)`. It must hold for `identity()` and be
     * monotone: once false for an interval, false for every longer one.
     * @param left First index of the interval, `0 .. size()`.
     * @param pred Predicate on the interval aggregate.
     * @return The largest `end` in `[left, size()]` with `pred(aggregate of [left, end))` true, so
     * `end` is the first index whose inclusion breaks `pred` (or `size()` when none does). Returns
     * `npos` when `left > size()`.
     * @note Complexity: O(log size()). Example: with a sum monoid of non-negative values,
     * `max_right(0, [k](int s) { return s < k; })` is the index where the prefix sum reaches `k`.
     */
    template <typename Predicate>
    size_type max_right(size_type left, Predicate pred) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (left > size_)
        {
            return npos;
        }
        if (left == size_)
        {
            return size_;
        }
        size_type node = base_ + left;
        value_type accumulated = Monoid::identity();
        do
        {
            while ((node & 1U) == 0U)
            {
                node >>= 1U;
            }
            if (!pred(Monoid::combine(accumulated, tree_[node])))
            {
                while (node < base_)
                {
                    node <<= 1U;
                    if (pred(Monoid::combine(accumulated, tree_[node])))
                    {
                        accumulated = Monoid::combine(accumulated, tree_[node]);
                        ++node;
                    }
                }
                return node - base_;
            }
            accumulated = Monoid::combine(accumulated, tree_[node]);
            ++node;
        } while (!detail::segment_tree_is_power_of_two(node));
        return size_;
    }

    /**
     * @brief Extends a half-open interval `[begin, end)` as far left as `pred` allows.
     * @tparam Predicate Callable as `bool(const value_type&)`; must hold for `identity()` and be
     * monotone as in `max_right()`.
     * @param end One past the last index of the interval, `0 .. size()`.
     * @param pred Predicate on the interval aggregate.
     * @return The smallest `begin` in `[0, end]` with `pred(aggregate of [begin, end))` true, or
     * `npos` when `end > size()`.
     * @note Complexity: O(log size()).
     */
    template <typename Predicate>
    size_type min_left(size_type end, Predicate pred) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (end > size_)
        {
            return npos;
        }
        if (end == 0U)
        {
            return 0U;
        }
        size_type node = base_ + end;
        value_type accumulated = Monoid::identity();
        do
        {
            --node;
            while ((node > 1U) && ((node & 1U) != 0U))
            {
                node >>= 1U;
            }
            if (!pred(Monoid::combine(tree_[node], accumulated)))
            {
                while (node < base_)
                {
                    node = (node << 1U) + 1U;
                    if (pred(Monoid::combine(tree_[node], accumulated)))
                    {
                        accumulated = Monoid::combine(tree_[node], accumulated);
                        --node;
                    }
                }
                return node + 1U - base_;
            }
            accumulated = Monoid::combine(tree_[node], accumulated);
        } while (!detail::segment_tree_is_power_of_two(node));
        return 0U;
    }

    /** @brief Removes every element. */
    void clear() CASTLE_NOEXCEPT
    {
        size_ = 0U;
        base_ = 1U;
        tree_[1U] = Monoid::identity();
    }

    /** @brief Returns the number of elements. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_;
    }

    /** @brief Returns `Capacity`. */
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Capacity;
    }

    /** @brief Reports whether the tree has no elements. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == 0U;
    }

private:
    /** @brief Sets the element count and the matching leaf base. */
    void resize(size_type count) CASTLE_NOEXCEPT
    {
        size_ = count;
        base_ = bit::next_power_of_two(count);
    }

    /** @brief Fills unused leaves with `identity()` and recomputes every internal node. */
    void rebuild() CASTLE_NOEXCEPT
    {
        for (size_type i = base_ + size_; i < (base_ << 1U); ++i)
        {
            tree_[i] = Monoid::identity();
        }
        for (size_type node = base_ - 1U; node > 0U; --node)
        {
            pull(node);
        }
    }

    /** @brief Recomputes `node` from its two children. */
    void pull(size_type node) CASTLE_NOEXCEPT
    {
        tree_[node] = Monoid::combine(tree_[node << 1U], tree_[(node << 1U) + 1U]);
    }

    array<value_type, static_leaf_capacity * 2U> tree_; // node k has children 2k, 2k + 1; leaves at base_
    size_type size_;
    size_type base_;
};

/**
 * @brief Segment tree with lazy propagation: range updates and range queries.
 * @tparam Policy Lazy policy: a monoid plus `tag_type`, `tag_identity()`, `apply(tag, value, length)`,
 * and `compose(newer, older)`. See `range_add_sum`, `range_add_min`, `range_add_max`,
 * `range_assign_sum`, `range_assign_min`, `range_assign_max`, or write your own.
 * @tparam Capacity Maximum number of elements. Must be greater than zero.
 * @note `apply()` must distribute over `combine()`: updating two aggregates and combining them must
 * equal combining first and then updating the merged aggregate with the summed length.
 * @note Queries push pending tags down, so they are not `const`.
 * @warning Out-of-range arguments are rejected in every build: mutators return
 * `status::out_of_range`, `get()`/`query()` return `identity()`, and the binary searches return `npos`.
 */
template <typename Policy, size_type Capacity>
class lazy_segment_tree
{
    static_assert(Capacity > 0U, "dsa::lazy_segment_tree requires a positive capacity");
    static_assert(Capacity <= (static_cast<castle::size_type>(-1) / 4U), "dsa::lazy_segment_tree capacity is too large");

public:
    using policy_type = Policy;
    using value_type = typename Policy::value_type;
    using tag_type = typename Policy::tag_type;
    using size_type = castle::size_type;

    /** @brief Compile-time maximum number of elements. */
    static CASTLE_CONSTEXPR size_type static_capacity = Capacity;
    /** @brief Leaf slots in the embedded storage, `next_power_of_two(Capacity)`. */
    static CASTLE_CONSTEXPR size_type static_leaf_capacity = bit::next_power_of_two_const<Capacity>::value;
    /** @brief Returned by `max_right()` / `min_left()` for an out-of-range bound. */
    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

    /** @brief Constructs an empty tree. Complexity: O(Capacity) for value-initializing storage. */
    lazy_segment_tree() CASTLE_NOEXCEPT
        : tree_{}, tags_{}, size_(0U), base_(1U), height_(0U)
    {
        tree_[1U] = Policy::identity();
    }

    /**
     * @brief Replaces the contents with `count` values and drops pending updates.
     * @return `status::ok`, `status::out_of_range` when `count > Capacity`, or
     * `status::invalid_argument` for a null source. The tree is unchanged on failure.
     * @note Complexity: O(count).
     */
    status build(CASTLE_CONST value_type* values, size_type count) CASTLE_NOEXCEPT
    {
        if (count > Capacity)
        {
            return status::out_of_range;
        }
        if ((count > 0U) && (values == nullptr))
        {
            return status::invalid_argument;
        }
        resize(count);
        for (size_type i = 0U; i < count; ++i)
        {
            tree_[base_ + i] = values[i];
        }
        rebuild();
        return status::ok;
    }

    /**
     * @brief Replaces the contents with `count` copies of `value` and drops pending updates.
     * @return `status::ok`, or `status::out_of_range` when `count > Capacity` (tree unchanged).
     * @note Complexity: O(count).
     */
    status assign(size_type count, CASTLE_CONST value_type& value = Policy::identity()) CASTLE_NOEXCEPT
    {
        if (count > Capacity)
        {
            return status::out_of_range;
        }
        resize(count);
        for (size_type i = 0U; i < count; ++i)
        {
            tree_[base_ + i] = value;
        }
        rebuild();
        return status::ok;
    }

    /**
     * @brief Replaces the element at `index`.
     * @return `status::ok` or `status::out_of_range`.
     * @note Complexity: O(log size()).
     */
    status set(size_type index, CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return status::out_of_range;
        }
        CASTLE_CONST size_type leaf = base_ + index;
        push_path(leaf);
        tree_[leaf] = value;
        for (size_type level = 1U; level <= height_; ++level)
        {
            pull(leaf >> level);
        }
        return status::ok;
    }

    /**
     * @brief Returns the element at `index`, or `identity()` when `index >= size()`.
     * @note Complexity: O(log size()).
     */
    value_type get(size_type index) CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return Policy::identity();
        }
        CASTLE_CONST size_type leaf = base_ + index;
        push_path(leaf);
        return tree_[leaf];
    }

    /**
     * @brief Applies `tag` to every element of the inclusive interval `[left, right]`.
     * @return `status::ok`, or `status::out_of_range` when `left > right` or `right >= size()`.
     * @note Complexity: O(log size()).
     */
    status apply(size_type left, size_type right, CASTLE_CONST tag_type& tag) CASTLE_NOEXCEPT
    {
        if ((left > right) || (right >= size_))
        {
            return status::out_of_range;
        }
        CASTLE_CONST size_type first = base_ + left;
        CASTLE_CONST size_type last = base_ + right + 1U;
        push_boundaries(first, last);

        size_type lo = first;
        size_type hi = last;
        size_type length = 1U;
        while (lo < hi)
        {
            if ((lo & 1U) != 0U)
            {
                apply_node(lo, tag, length);
                ++lo;
            }
            if ((hi & 1U) != 0U)
            {
                --hi;
                apply_node(hi, tag, length);
            }
            lo >>= 1U;
            hi >>= 1U;
            length <<= 1U;
        }

        for (size_type level = 1U; level <= height_; ++level)
        {
            if (((first >> level) << level) != first)
            {
                pull(first >> level);
            }
            if (((last >> level) << level) != last)
            {
                pull((last - 1U) >> level);
            }
        }
        return status::ok;
    }

    /**
     * @brief Returns the aggregate of the inclusive interval `[left, right]`, combined left to right.
     * @return Aggregate, or `identity()` when `left > right` or `right >= size()`.
     * @note Complexity: O(log size()).
     */
    value_type query(size_type left, size_type right) CASTLE_NOEXCEPT
    {
        if ((left > right) || (right >= size_))
        {
            return Policy::identity();
        }
        size_type first = base_ + left;
        size_type last = base_ + right + 1U;
        push_boundaries(first, last);

        value_type left_result = Policy::identity();
        value_type right_result = Policy::identity();
        while (first < last)
        {
            if ((first & 1U) != 0U)
            {
                left_result = Policy::combine(left_result, tree_[first]);
                ++first;
            }
            if ((last & 1U) != 0U)
            {
                --last;
                right_result = Policy::combine(tree_[last], right_result);
            }
            first >>= 1U;
            last >>= 1U;
        }
        return Policy::combine(left_result, right_result);
    }

    /** @brief Returns the aggregate of all elements (`identity()` when empty). Complexity: O(1). */
    value_type all() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return tree_[1U]; // the root; identity() for an empty tree
    }

    /**
     * @brief Extends a half-open interval `[left, end)` as far right as `pred` allows.
     * @return The largest `end` in `[left, size()]` with `pred(aggregate of [left, end))` true, or
     * `npos` when `left > size()`. Same contract as `segment_tree::max_right()`.
     * @note Complexity: O(log size()).
     */
    template <typename Predicate>
    size_type max_right(size_type left, Predicate pred) CASTLE_NOEXCEPT
    {
        if (left > size_)
        {
            return npos;
        }
        if (left == size_)
        {
            return size_;
        }
        size_type node = base_ + left;
        push_path(node);
        size_type level = 0U; // height of `node` above the leaves
        value_type accumulated = Policy::identity();
        do
        {
            while ((node & 1U) == 0U)
            {
                node >>= 1U;
                ++level;
            }
            if (!pred(Policy::combine(accumulated, tree_[node])))
            {
                while (node < base_)
                {
                    push(node, level);
                    node <<= 1U;
                    --level;
                    if (pred(Policy::combine(accumulated, tree_[node])))
                    {
                        accumulated = Policy::combine(accumulated, tree_[node]);
                        ++node;
                    }
                }
                return node - base_;
            }
            accumulated = Policy::combine(accumulated, tree_[node]);
            ++node;
        } while (!detail::segment_tree_is_power_of_two(node));
        return size_;
    }

    /**
     * @brief Extends a half-open interval `[begin, end)` as far left as `pred` allows.
     * @return The smallest `begin` in `[0, end]` with `pred(aggregate of [begin, end))` true, or
     * `npos` when `end > size()`. Same contract as `segment_tree::min_left()`.
     * @note Complexity: O(log size()).
     */
    template <typename Predicate>
    size_type min_left(size_type end, Predicate pred) CASTLE_NOEXCEPT
    {
        if (end > size_)
        {
            return npos;
        }
        if (end == 0U)
        {
            return 0U;
        }
        size_type node = base_ + end;
        push_path(node - 1U);
        size_type level = 0U; // height of `node` above the leaves
        value_type accumulated = Policy::identity();
        do
        {
            --node;
            while ((node > 1U) && ((node & 1U) != 0U))
            {
                node >>= 1U;
                ++level;
            }
            if (!pred(Policy::combine(tree_[node], accumulated)))
            {
                while (node < base_)
                {
                    push(node, level);
                    node = (node << 1U) + 1U;
                    --level;
                    if (pred(Policy::combine(tree_[node], accumulated)))
                    {
                        accumulated = Policy::combine(tree_[node], accumulated);
                        --node;
                    }
                }
                return node + 1U - base_;
            }
            accumulated = Policy::combine(tree_[node], accumulated);
        } while (!detail::segment_tree_is_power_of_two(node));
        return 0U;
    }

    /** @brief Removes every element. */
    void clear() CASTLE_NOEXCEPT
    {
        size_ = 0U;
        base_ = 1U;
        height_ = 0U;
        tree_[1U] = Policy::identity();
    }

    /** @brief Returns the number of elements. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_;
    }

    /** @brief Returns `Capacity`. */
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Capacity;
    }

    /** @brief Reports whether the tree has no elements. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == 0U;
    }

private:
    /** @brief Sets the element count, the matching leaf base, and the tree height. */
    void resize(size_type count) CASTLE_NOEXCEPT
    {
        size_ = count;
        base_ = bit::next_power_of_two(count);
        height_ = 0U;
        while ((static_cast<size_type>(1U) << height_) < base_)
        {
            ++height_;
        }
    }

    /** @brief Fills unused leaves with `identity()`, recomputes internal nodes, and clears tags. */
    void rebuild() CASTLE_NOEXCEPT
    {
        for (size_type i = base_ + size_; i < (base_ << 1U); ++i)
        {
            tree_[i] = Policy::identity();
        }
        for (size_type node = base_ - 1U; node > 0U; --node)
        {
            pull(node);
            tags_[node] = Policy::tag_identity();
        }
    }

    /** @brief Recomputes `node` from its two children. */
    void pull(size_type node) CASTLE_NOEXCEPT
    {
        tree_[node] = Policy::combine(tree_[node << 1U], tree_[(node << 1U) + 1U]);
    }

    /** @brief Updates the aggregate of `node`, which covers `length` leaves, and queues `tag` for its children. */
    void apply_node(size_type node, CASTLE_CONST tag_type& tag, size_type length) CASTLE_NOEXCEPT
    {
        tree_[node] = Policy::apply(tag, tree_[node], length);
        if (node < base_)
        {
            tags_[node] = Policy::compose(tag, tags_[node]);
        }
    }

    /** @brief Hands the pending tag of internal `node` at height `level` down to its children. */
    void push(size_type node, size_type level) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type child_length = static_cast<size_type>(1U) << (level - 1U);
        apply_node(node << 1U, tags_[node], child_length);
        apply_node((node << 1U) + 1U, tags_[node], child_length);
        tags_[node] = Policy::tag_identity();
    }

    /** @brief Pushes every pending tag on the path from the root down to `leaf`'s parent. */
    void push_path(size_type leaf) CASTLE_NOEXCEPT
    {
        for (size_type level = height_; level > 0U; --level)
        {
            push(leaf >> level, level);
        }
    }

    /** @brief Pushes tags above the edges of the leaf interval `[first, last)`. */
    void push_boundaries(size_type first, size_type last) CASTLE_NOEXCEPT
    {
        for (size_type level = height_; level > 0U; --level)
        {
            if (((first >> level) << level) != first)
            {
                push(first >> level, level);
            }
            if (((last >> level) << level) != last)
            {
                push((last - 1U) >> level, level);
            }
        }
    }

    array<value_type, static_leaf_capacity * 2U> tree_; // node k has children 2k, 2k + 1; leaves at base_
    array<tag_type, static_leaf_capacity> tags_;       // pending update of internal node k
    size_type size_;
    size_type base_;
    size_type height_;
};

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_SEGMENT_TREE_HPP
