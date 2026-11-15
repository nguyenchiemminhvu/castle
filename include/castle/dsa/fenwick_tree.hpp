// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file fenwick_tree.hpp
 * @brief Fixed-capacity Fenwick (binary indexed) tree for prefix sums with point updates.
 *
 * Use `fenwick_tree<T, Capacity>` when a sequence of additive values changes one element at a time
 * and prefix or range sums are queried often: frequency tables, running totals, inversion counts,
 * order statistics, or counting range sums over compressed coordinates (LeetCode 327). Updates and
 * queries are O(log n); the tree stores exactly one `T` per element and never allocates.
 *
 * For aggregates that cannot be undone by subtraction (minimum, maximum, gcd) or for range updates
 * with range queries, use `segment_tree` / `lazy_segment_tree` instead.
 *
 * @code
 * castle::dsa::fenwick_tree<int32_t, 8U> sums;
 * const int32_t values[] = {2, 3, 5, 7};
 * sums.build(values, 4U);
 * sums.add(1U, 10);                       // values = {2, 13, 5, 7}
 * int32_t total = sums.range_sum(1U, 2U); // 18
 * @endcode
 *
 * @see https://cp-algorithms.com/data_structures/fenwick.html
 */
#ifndef CASTLE_DSA_FENWICK_TREE_HPP
#define CASTLE_DSA_FENWICK_TREE_HPP

#include "castle/bit/bit_math.hpp"
#include "castle/bit/bit_utils.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/array.hpp"

namespace castle
{
namespace dsa
{

/**
 * @brief Fenwick tree over up to `Capacity` elements.
 * @tparam T Additive value type: default construction must yield zero, and `+=`, `-`, and `<` must
 * be defined. Arithmetic overflow follows the rules of `T`.
 * @tparam Capacity Maximum number of elements. Must be greater than zero.
 * @note Element indices are zero-based. `prefix_sum(count)` sums `[0, count)`; `range_sum(left,
 * right)` sums the inclusive interval `[left, right]`.
 * @note Node `i` (one-based) stores the sum of the `lowbit(i)` elements ending at element `i - 1`,
 * where `lowbit(i)` is the lowest set bit of `i`.
 * @warning Out-of-range arguments are rejected in every build: mutators return
 * `status::out_of_range`, queries return `T()`. Check against `size()` first when a zero result
 * must be told apart from an invalid request.
 */
template <typename T, size_type Capacity>
class fenwick_tree
{
    static_assert(Capacity > 0U, "dsa::fenwick_tree requires a positive capacity");
    static_assert(Capacity < static_cast<castle::size_type>(-1) / 2U, "dsa::fenwick_tree capacity is too large");

public:
    using value_type = T;
    using size_type = castle::size_type;

    /** @brief Compile-time maximum number of elements. */
    static CASTLE_CONSTEXPR size_type static_capacity = Capacity;

    /** @brief Index returned by `lower_bound()` when no prefix reaches the target. */
    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

    /**
     * @brief Constructs `count` zero elements.
     * @param count Number of elements, `0 .. Capacity`. Defaults to 0.
     * @note Complexity: O(Capacity).
     * @warning `count > Capacity` reports through `CASTLE_ASSERT` and is clamped to `Capacity`.
     */
    explicit fenwick_tree(size_type count = 0U) CASTLE_NOEXCEPT
        : tree_{}, size_(0U)
    {
        CASTLE_ASSERT(count <= Capacity, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::dsa::fenwick_tree: element count exceeds capacity"));
        size_ = (count <= Capacity) ? count : Capacity; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Replaces the contents with `count` zero elements.
     * @return `status::ok`, or `status::out_of_range` when `count > Capacity` (tree unchanged).
     * @note Complexity: O(count).
     */
    status assign(size_type count) CASTLE_NOEXCEPT
    {
        if (count > Capacity)
        {
            return status::out_of_range;
        }
        for (size_type i = 0U; i < count; ++i)
        {
            tree_[i] = T();
        }
        size_ = count;
        return status::ok;
    }

    /**
     * @brief Replaces the contents with `count` values.
     * @param values Source values; may be null only when `count == 0`.
     * @param count Number of values.
     * @return `status::ok`, `status::out_of_range` when `count > Capacity`, or
     * `status::invalid_argument` for a null source. The tree is unchanged on failure.
     * @note Complexity: O(count). Each node pushes its total to its parent once.
     */
    status build(CASTLE_CONST T* values, size_type count) CASTLE_NOEXCEPT
    {
        if (count > Capacity)
        {
            return status::out_of_range;
        }
        if ((count > 0U) && (values == nullptr))
        {
            return status::invalid_argument;
        }

        for (size_type i = 0U; i < count; ++i)
        {
            tree_[i] = values[i];
        }
        for (size_type node = 1U; node <= count; ++node)
        {
            CASTLE_CONST size_type parent = node + lowbit(node);
            if (parent <= count)
            {
                tree_[parent - 1U] += tree_[node - 1U];
            }
        }
        size_ = count;
        return status::ok;
    }

    /**
     * @brief Adds `delta` to the element at `index`.
     * @return `status::ok` or `status::out_of_range`.
     * @note Complexity: O(log size()).
     */
    status add(size_type index, CASTLE_CONST T& delta) CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return status::out_of_range;
        }
        for (size_type node = index + 1U; node <= size_; node += lowbit(node))
        {
            tree_[node - 1U] += delta;
        }
        return status::ok;
    }

    /**
     * @brief Replaces the element at `index` with `value`.
     * @return `status::ok` or `status::out_of_range`.
     * @note Complexity: O(log size()); the old value is read with `get()` and the difference added.
     */
    status set(size_type index, CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return status::out_of_range;
        }
        return add(index, value - get(index));
    }

    /**
     * @brief Returns the element at `index`.
     * @return Element value, or `T()` when `index >= size()`.
     * @note Complexity: O(log size()).
     */
    T get(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return T();
        }
        return range_sum(index, index);
    }

    /**
     * @brief Returns the sum of the first `count` elements, `[0, count)`.
     * @param count Number of leading elements; may equal `size()`.
     * @return Prefix sum, or `T()` when `count > size()`.
     * @note Complexity: O(log size()).
     */
    T prefix_sum(size_type count) CASTLE_CONST CASTLE_NOEXCEPT
    {
        T result = T();
        if (count > size_)
        {
            return result;
        }
        for (size_type node = count; node > 0U; node -= lowbit(node))
        {
            result += tree_[node - 1U];
        }
        return result;
    }

    /**
     * @brief Returns the sum of the inclusive interval `[left, right]`.
     * @return Interval sum, or `T()` when `left > right` or `right >= size()`.
     * @note Complexity: O(log size()).
     */
    T range_sum(size_type left, size_type right) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if ((left > right) || (right >= size_))
        {
            return T();
        }
        return prefix_sum(right + 1U) - prefix_sum(left);
    }

    /**
     * @brief Finds the first index whose prefix sum reaches `target`.
     * @param target Value to reach.
     * @return Smallest `index` with `prefix_sum(index + 1) >= target`, or `npos` when even the total
     * stays below `target`. Returns 0 for `target <= T()` on a non-empty tree.
     * @note Requires every element to be non-negative, so prefix sums never decrease. With a
     * frequency table this returns the k-th smallest value for `target = k` (one-based).
     * @note Complexity: O(log size()); a single top-down descent with no repeated prefix sums.
     */
    size_type lower_bound(T target) CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type position = 0U; // nodes 1 .. position hold a total below target
        for (size_type step = bit::previous_power_of_two(size_); step > 0U; step >>= 1U)
        {
            CASTLE_CONST size_type next = position + step;
            if ((next <= size_) && (tree_[next - 1U] < target))
            {
                position = next;
                target = target - tree_[next - 1U];
            }
        }
        return (position < size_) ? position : npos;
    }

    /** @brief Removes every element. */
    void clear() CASTLE_NOEXCEPT
    {
        size_ = 0U;
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
    /** @brief Lowest set bit of a one-based node index; the number of elements the node covers. */
    static CASTLE_CONSTEXPR size_type lowbit(size_type node) CASTLE_NOEXCEPT
    {
        return bit::extract_lowest_set_bit(node);
    }

    array<T, Capacity> tree_; // tree_[i] is node i + 1 of the one-based Fenwick layout
    size_type size_;
};

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_FENWICK_TREE_HPP
