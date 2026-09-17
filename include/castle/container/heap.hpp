// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file heap.hpp
 * @brief Fixed-capacity binary heap with deterministic in-object storage.
 *
 * Use this container when prioritized insertion/removal is required without dynamic allocation.
 * The comparator defines whether the root is the greatest or smallest element, storage for `N`
 * values is embedded directly in the heap, and `push()`, `emplace()`, `pop()`, `remove_at()`,
 * and `replace_top()` run in O(log N). Observers such as `top()`, `size()`, and `empty()` are O(1).
 *
 * @note The heap never reallocates, throws, or depends on the STL.
 * @warning Insertions return `status::full` once all `N` slots are occupied.
 */
#ifndef CASTLE_CONTAINER_HEAP_HPP
#define CASTLE_CONTAINER_HEAP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/error/status.hpp"
#include "castle/memory/object.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/static_storage.hpp"
#include "castle/utility/compare.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"

namespace castle
{
namespace container
{

/**
 * @brief Fixed-capacity binary heap parameterized by a strict-priority comparator.
 * @tparam T Element type.
 * @tparam N Maximum number of stored elements.
 * @tparam Compare Comparator that returns `true` when the left operand has higher priority.
 * @note Elements are stored in a contiguous array using the usual binary-heap parent/child layout.
 * @warning The heap is non-copyable and non-movable because it owns in-place storage.
 */
template <typename T, size_type N, typename Compare>
class basic_heap
{
    static_assert(N > 0U, "basic_heap requires a positive capacity");
    static_assert(meta::is_destructible<T>::value,
                  "basic_heap<T,N> requires a destructible T");

public:
    using value_type = T;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using reference = T&;
    using const_reference = CASTLE_CONST T&;
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    /**
     * @brief Constructs an empty heap with a default-constructed comparator.
     * @note Complexity is O(1).
     */
    basic_heap() CASTLE_NOEXCEPT : size_(0U), compare_() {}

    /**
     * @brief Constructs an empty heap with a caller-supplied comparator.
     * @param compare Comparator instance used for heap ordering.
     * @note Complexity is O(1).
     */
    explicit basic_heap(CASTLE_CONST Compare& compare) CASTLE_NOEXCEPT : size_(0U), compare_(compare) {}

    /**
     * @brief Constructs a heap by inserting initializer-list elements one by one.
     * @param list Source elements.
     * @note Complexity is O(list.size() * log N).
     * @warning Oversized initializer lists trigger `CASTLE_ASSERT`.
     */
    basic_heap(initializer_list<T> list) CASTLE_NOEXCEPT : size_(0U), compare_()
    {
        for (CASTLE_CONST T& value : list)
        {
            if (full())
            {
                break;
            }
            push(value);
        }
    }

    /**
     * @brief Destroys all stored elements.
     * @note Complexity is O(size()).
     */
    ~basic_heap() CASTLE_NOEXCEPT { clear(); }

    basic_heap(CASTLE_CONST basic_heap&) CASTLE_DELETE;
    basic_heap& operator=(CASTLE_CONST basic_heap&) CASTLE_DELETE;
    basic_heap(basic_heap&&) CASTLE_DELETE;
    basic_heap& operator=(basic_heap&&) CASTLE_DELETE;

    /**
     * @brief Returns the fixed compile-time capacity.
     * @return `N`.
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the current element count.
     * @return Number of stored elements in O(1).
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Tests whether the heap has no elements.
     * @return `true` when `size() == 0`.
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Tests whether the heap has reached capacity.
     * @return `true` when `size() == capacity()`.
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    /**
     * @brief Returns the number of free element slots.
     * @return `capacity() - size()` in O(1).
     */
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }

    /**
     * @brief Returns the highest-priority element.
     * @return Reference to the heap root.
     * @warning The heap must not be empty.
     */
    reference top() CASTLE_NOEXCEPT { return *pointer_at(0U); }

    /**
     * @brief Returns the highest-priority element.
     * @return Const reference to the heap root.
     * @warning The heap must not be empty.
     */
    const_reference top() CASTLE_CONST CASTLE_NOEXCEPT { return *pointer_at(0U); }

    /**
     * @brief Inserts a copy of `value`.
     * @param value Element to insert.
     * @return `status::ok` on success or `status::full` when no slot remains.
     * @note Complexity is O(log N) because the new element may sift toward the root.
     */
    status push(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return status::full;
        }

        memory::construct_at<T>(storage_.address(size_), value);
        ++size_;
        sift_up(size_ - 1U);
        return status::ok;
    }

    /**
     * @brief Inserts `value` by move construction.
     * @param value Element to insert.
     * @return `status::ok` on success or `status::full` when no slot remains.
     * @note Complexity is O(log N).
     */
    status push(T&& value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return status::full;
        }

        memory::construct_at<T>(storage_.address(size_), CASTLE_MOVE(value));
        ++size_;
        sift_up(size_ - 1U);
        return status::ok;
    }

    /**
     * @brief Constructs a new element in-place and inserts it into the heap.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to `T`.
     * @return `status::ok` on success or `status::full` when no slot remains.
     * @note Complexity is O(log N).
     */
    template <typename... Args>
    status emplace(Args&&... args) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return status::full;
        }

        memory::construct_at<T>(storage_.address(size_), CASTLE_FORWARD<Args>(args)...);
        ++size_;
        sift_up(size_ - 1U);
        return status::ok;
    }

    /**
     * @brief Moves the top element into `out` and removes it from the heap.
     * @param out Destination for the removed element.
     * @return `status::ok` on success or `status::empty` when the heap has no elements.
     * @note Complexity is O(log N).
     */
    status pop(T& out) CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }

        out = CASTLE_MOVE(*pointer_at(0U));
        return pop();
    }

    /**
     * @brief Removes the top element.
     * @return `status::ok` on success or `status::empty` when the heap has no elements.
     * @note Complexity is O(log N) because the replacement root may sift downward.
     */
    status pop() CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }

        CASTLE_CONST size_type last = size_ - 1U;
        if (last != 0U)
        {
            *pointer_at(0U) = CASTLE_MOVE(*pointer_at(last));
        }
        memory::destroy_at(pointer_at(last));
        --size_;
        if (size_ > 0U)
        {
            sift_down(0U);
        }
        return status::ok;
    }

    /**
     * @brief Removes the element at `index`.
     * @param index Zero-based heap-array index.
     * @return `status::ok` on success or `status::out_of_range` when `index >= size()`.
     * @note Complexity is O(log N) because the moved replacement may sift up or down.
     * @warning `index` refers to the internal heap order, not sorted order.
     */
    status remove_at(size_type index) CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return status::out_of_range;
        }

        CASTLE_CONST size_type last = size_ - 1U;
        if (index != last)
        {
            *pointer_at(index) = CASTLE_MOVE(*pointer_at(last));
        }
        memory::destroy_at(pointer_at(last));
        --size_;
        if (index < size_)
        {
            if ((index > 0U) && higher_priority(index, parent(index))) // LCOV_EXCL_BR_LINE
            {
                sift_up(index);
            }
            else
            {
                sift_down(index);
            }
        }
        return status::ok;
    }

    /**
     * @brief Replaces the top element by moving from `value`.
     * @param value Replacement value.
     * @return `status::ok` on success or `status::empty` when the heap has no elements.
     * @note Complexity is O(log N).
     */
    status replace_top(T& value) CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }
        *pointer_at(0U) = CASTLE_MOVE(value);
        sift_down(0U);
        return status::ok;
    }

    /**
     * @brief Replaces the top element with a copy of `value`.
     * @param value Replacement value.
     * @return `status::ok` on success or `status::empty` when the heap has no elements.
     * @note Complexity is O(log N).
     */
    status replace_top(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }
        *pointer_at(0U) = value;
        sift_down(0U);
        return status::ok;
    }

    /**
     * @brief Destroys all stored elements.
     * @note Complexity is O(size()).
     */
    void clear() CASTLE_NOEXCEPT
    {
        for (size_type i = size_; i > 0U; --i)
        {
            memory::destroy_at(pointer_at(i - 1U));
        }
        size_ = 0U;
    }

    /**
     * @brief Returns an iterator to the first heap-array slot.
     * @return Pointer to the root or `nullptr` when empty.
     * @note Iteration visits internal heap order, not sorted order.
     */
    iterator begin() CASTLE_NOEXCEPT { return size_ == 0U ? nullptr : pointer_at(0U); }

    /**
     * @brief Returns a const iterator to the first heap-array slot.
     * @return Pointer to the root or `nullptr` when empty.
     * @note Iteration visits internal heap order, not sorted order.
     */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U ? nullptr : pointer_at(0U); }

    /**
     * @brief Returns an iterator one past the last heap-array slot.
     * @return End pointer or `nullptr` when empty.
     */
    iterator end() CASTLE_NOEXCEPT { return size_ == 0U ? nullptr : pointer_at(size_); }

    /**
     * @brief Returns a const iterator one past the last heap-array slot.
     * @return Const end pointer or `nullptr` when empty.
     */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U ? nullptr : pointer_at(size_); }

private:
    /**
     * @brief Retrieves a pointer to the element at the given index in the heap array.
     * @param index Index of the element to retrieve.
     * @return Pointer to the element at the given index, or `nullptr` if the index is out of bounds.
     */
    pointer pointer_at(size_type index) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<T>(storage_.address(index));
    }

    /**
     * @brief Retrieves a pointer to the element at the given index in the heap array (const version).
     * @param index Index of the element to retrieve.
     * @return Pointer to the element at the given index, or `nullptr` if the index is out of bounds.
     */
    CASTLE_CONST T* pointer_at(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return memory::object_from_address<CASTLE_CONST T>(storage_.address(index));
    }

    /**
     * @brief Computes the index of the parent of the given node in the heap array.
     * @param index Index of the node whose parent is to be computed.
     * @return Index of the parent node.
     */
    static CASTLE_CONSTEXPR size_type parent(size_type index) CASTLE_NOEXCEPT
    {
        return (index - 1U) / 2U;
    }

    /**
     * @brief Computes the index of the left child of the given node in the heap array.
     * @param index Index of the node whose left child is to be computed.
     * @return Index of the left child node.
     */
    static CASTLE_CONSTEXPR size_type left(size_type index) CASTLE_NOEXCEPT
    {
        return index * 2U + 1U;
    }

    /**
     * @brief Determines if the element at the first index has higher priority than the element at the second index according to the heap's comparator.
     * @param lhs Index of the first element.
     * @param rhs Index of the second element.
     * @return `true` if the element at `lhs` has higher priority than the element at `rhs`, `false` otherwise.
     */
    bool higher_priority(size_type lhs, size_type rhs) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return compare_(*pointer_at(lhs), *pointer_at(rhs));
    }

    /**
     * @brief Moves the element at the given index up the heap until the heap property is restored.
     * @param index Index of the element to sift up.
     */
    void sift_up(size_type index) CASTLE_NOEXCEPT
    {
        while (index > 0U)
        {
            CASTLE_CONST size_type p = parent(index);
            if (!higher_priority(index, p))
            {
                break;
            }
            swap_values(index, p);
            index = p;
        }
    }

    /**
     * @brief Moves the element at the given index down the heap until the heap property is restored.
     * @param index Index of the element to sift down.
     */
    void sift_down(size_type index) CASTLE_NOEXCEPT
    {
        for (;;)
        {
            CASTLE_CONST size_type child = left(index);
            if (child >= size_)
            {
                break;
            }
            size_type selected = child;
            CASTLE_CONST size_type right = child + 1U;
            if ((right < size_) && higher_priority(right, child))
            {
                selected = right;
            }
            if (!higher_priority(selected, index))
            {
                break;
            }
            swap_values(selected, index);
            index = selected;
        }
    }

    /**
     * @brief Swaps the values of the elements at the given indices.
     * @param lhs Index of the first element.
     * @param rhs Index of the second element.
     * @brief Swaps the values of the elements at the given indices.
     */
    void swap_values(size_type lhs, size_type rhs) CASTLE_NOEXCEPT
    {
        T temp(CASTLE_MOVE(*pointer_at(lhs)));
        *pointer_at(lhs) = CASTLE_MOVE(*pointer_at(rhs));
        *pointer_at(rhs) = CASTLE_MOVE(temp);
    }

    size_type size_;
    Compare compare_;
    memory::static_storage<T, N> storage_;
};

/**
 * @brief Max-heap alias whose default comparator keeps the greatest value at the root.
 * @tparam T Element type.
 * @tparam N Maximum number of stored elements.
 * @tparam Compare Comparator type, defaulting to `castle::greater<T>`.
 */
template <typename T, size_type N, typename Compare = castle::greater<T>>
using max_heap = basic_heap<T, N, Compare>;

/**
 * @brief Min-heap alias whose default comparator keeps the smallest value at the root.
 * @tparam T Element type.
 * @tparam N Maximum number of stored elements.
 * @tparam Compare Comparator type, defaulting to `castle::less<T>`.
 */
template <typename T, size_type N, typename Compare = castle::less<T>>
using min_heap = basic_heap<T, N, Compare>;

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_HEAP_HPP
