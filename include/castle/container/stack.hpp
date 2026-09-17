// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file stack.hpp
 * @brief Fixed-capacity LIFO buffer with inline storage.
 *
 * Use `stack_buffer<T, N>` when values should be pushed and popped in
 * last-in/first-out order with deterministic memory usage. Storage is embedded,
 * capacity is fixed by `N`, and all single-element operations are O(1).
 * `push()` reports a full buffer without overwriting, while `force_push()`
 * explicitly replaces the current top element when the buffer is already full.
 *
 * @code
 * castle::container::stack_buffer<int, 4U> stack;
 * stack.push(1);
 * stack.force_push(2);
 * int top = stack.top();
 * @endcode
 */
#ifndef CASTLE_CONTAINER_STACK_HPP
#define CASTLE_CONTAINER_STACK_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/utility/move.hpp"
#include "castle/container/array.hpp"
#include "castle/container/initializer_list.hpp"

namespace castle
{
namespace container
{

/**
 * @brief Fixed-capacity LIFO buffer with inline storage.
 * @tparam T Element type. Must be trivially copyable and trivially destructible.
 * @tparam N Stack capacity. Must be greater than `0`.
 * @note `push()`, `force_push()`, `pop()`, `peek()`, and `top()` are O(1).
 * Bulk operations are O(k) where `k` is the number of transferred elements.
 * @note `peek(0, out)` addresses the oldest element, while `top()` and
 * `peek(size() - 1, out)` address the newest element.
 * @warning `top()` requires the stack to be non-empty.
 */
template <typename T, size_type N>
class stack_buffer
{
    static_assert(N > 0U, "stack_buffer capacity must be non-zero");
    static_assert(meta::is_trivially_copyable<T>::value,
                  "stack_buffer<T,N> requires T to be trivially copyable");
    static_assert(meta::is_trivially_destructible<T>::value,
                  "stack_buffer<T,N> requires T to be trivially destructible");

public:
    using value_type      = T;
    using size_type       = castle::size_type;
    using difference_type = castle::difference_type;
    using reference       = T&;
    using const_reference = CASTLE_CONST T&;

    /**
     * @brief Constructs an empty stack buffer.
     * @note Complexity: O(1).
     */
    stack_buffer() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Constructs a stack buffer by pushing initializer-list elements in order.
     * @param list Values pushed from first to last.
     * @note Complexity: O(list.size()).
     * @warning `list.size()` must not exceed `N`; the constructor reports a
     * violated precondition with `CASTLE_ASSERT`.
     */
    stack_buffer(initializer_list<T> list) CASTLE_NOEXCEPT
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
     * @brief Copy construction is disabled.
     * @param other Source stack.
     * @warning Stack buffers are non-copyable.
     */
    stack_buffer(CASTLE_CONST stack_buffer&)            CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @param other Source stack.
     * @return This declaration is deleted.
     * @warning Stack buffers are non-copyable.
     */
    stack_buffer& operator=(CASTLE_CONST stack_buffer&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @param other Source stack.
     * @warning Stack buffers are non-movable.
     */
    stack_buffer(stack_buffer&&)                        CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @param other Source stack.
     * @return This declaration is deleted.
     * @warning Stack buffers are non-movable.
     */
    stack_buffer& operator=(stack_buffer&&)             CASTLE_DELETE;

    /**
     * @brief Returns the compile-time capacity.
     * @return Maximum number of stored elements.
     * @note Complexity: O(1).
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the number of stored elements.
     * @return Current element count.
     * @note Complexity: O(1).
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Reports whether the stack is empty.
     * @return `true` when `size() == 0`.
     * @note Complexity: O(1).
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Reports whether the stack is full.
     * @return `true` when `size() == capacity()`.
     * @note Complexity: O(1).
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    /**
     * @brief Returns the remaining free capacity.
     * @return `capacity() - size()`.
     * @note Complexity: O(1).
     */
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }

    /**
     * @brief Removes all elements.
     * @note Complexity: O(1).
     */
    void clear() CASTLE_NOEXCEPT
    {
        head_ = 0U;
        size_ = 0U;
    }

    /**
     * @brief Pushes a copied value onto the top of the stack.
     * @param value Value to push.
     * @return `true` on success or `false` when the stack is full.
     * @note Complexity: O(1).
     */
    bool push(CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return false;
        }
        data_[head_] = value;
        head_ = advance(head_);
        ++size_;
        return true;
    }

    /**
     * @brief Pushes a moved value onto the top of the stack.
     * @param value Value to move.
     * @return `true` on success or `false` when the stack is full.
     * @note Complexity: O(1).
     */
    bool push(value_type&& value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return false;
        }
        data_[head_] = castle::move(value);
        head_ = advance(head_);
        ++size_;
        return true;
    }

    /**
     * @brief Pushes a copied value, replacing the current top element when full.
     * @param value Value to place on the top of the stack.
     * @return `true` when an existing top element was replaced, otherwise `false`.
     * @note Complexity: O(1).
     */
    bool force_push(CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST bool evicted = full();
        if (evicted)
        {
            head_ = retreat(head_);
            data_[head_] = value;
            head_ = advance(head_);
        }
        else
        {
            data_[head_] = value;
            head_ = advance(head_);
            ++size_;
        }
        return evicted;
    }

    /**
     * @brief Pushes a moved value, replacing the current top element when full.
     * @param value Value to move onto the top of the stack.
     * @return `true` when an existing top element was replaced, otherwise `false`.
     * @note Complexity: O(1).
     */
    bool force_push(value_type&& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST bool evicted = full();
        if (evicted)
        {
            head_ = retreat(head_);
            data_[head_] = castle::move(value);
            head_ = advance(head_);
        }
        else
        {
            data_[head_] = castle::move(value);
            head_ = advance(head_);
            ++size_;
        }
        return evicted;
    }

    /**
     * @brief Pops the top element into `out`.
     * @param out Destination for the removed value.
     * @return `true` on success or `false` when the stack is empty.
     * @note Complexity: O(1).
     */
    bool pop(value_type& out) CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return false;
        }
        head_ = retreat(head_);
        out = data_[head_];
        --size_;
        return true;
    }

    /**
     * @brief Pops and discards the top element.
     * @return `true` on success or `false` when the stack is empty.
     * @note Complexity: O(1).
     */
    bool pop() CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return false;
        }
        head_ = retreat(head_);
        --size_;
        return true;
    }

    /**
     * @brief Copies an element at logical position `index` without removing it.
     * @param index Zero-based index where `0` is the oldest element and `size() - 1` is the newest.
     * @param out Destination for the copied value.
     * @return `true` when `index < size()`, otherwise `false`.
     * @note Complexity: O(1).
     */
    bool peek(size_type index, value_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return false;
        }
        out = data_[wrap(head_ + N - size_ + index)];
        return true;
    }

    /**
     * @brief Returns the newest stored element.
     * @return Mutable reference to the current top element.
     * @note Complexity: O(1).
     * @warning The stack must not be empty.
     */
    reference top() CASTLE_NOEXCEPT { return data_[retreat(head_)]; }

    /**
     * @brief Returns the newest stored element.
     * @return Const reference to the current top element.
     * @note Complexity: O(1).
     * @warning The stack must not be empty.
     */
    const_reference top() CASTLE_CONST CASTLE_NOEXCEPT { return data_[retreat(head_)]; }

    /**
     * @brief Pushes up to `max` values from `src` in order.
     * @param src Source array of values to push.
     * @param max Maximum number of values to push.
     * @return Number of values actually pushed.
     * @note Complexity: O(min(max, available())).
     * @warning Returns `0` immediately when `src == nullptr`.
     */
    size_type push_bulk(CASTLE_CONST value_type* src, size_type max) CASTLE_NOEXCEPT
    {
        if (src == nullptr)
        {
            return 0U;
        }
        size_type written = 0U;
        while ((written < max) && !full()) // LCOV_EXCL_BR_LINE
        {
            data_[head_] = src[written];
            head_ = advance(head_);
            ++size_;
            ++written;
        }
        return written;
    }

    /**
     * @brief Pops up to `max` values into `dst` in LIFO order.
     * @param dst Destination array for removed values.
     * @param max Maximum number of values to pop.
     * @return Number of values actually popped.
     * @note Complexity: O(min(max, size())).
     * @warning Returns `0` immediately when `dst == nullptr`.
     */
    size_type pop_bulk(value_type* dst, size_type max) CASTLE_NOEXCEPT
    {
        if (dst == nullptr)
        {
            return 0U;
        }
        size_type read = 0U;
        while ((read < max) && !empty())
        {
            head_ = retreat(head_);
            dst[read] = data_[head_];
            --size_;
            ++read;
        }
        return read;
    }

private:
    /**
     * @brief Wraps the given index according to the buffer size.
     * @param index Index to wrap.
     * @return Wrapped index within the buffer bounds.
     */
    static CASTLE_CONSTEXPR size_type wrap(size_type index) CASTLE_NOEXCEPT
    {
        return meta::is_power_of_two<N>::value
                   ? (index & (N - 1U))
                   : (index % N);
    }

    /**
     * @brief Advances the given index by one position in the circular buffer.
     * @param index Current index in the circular buffer.
     * @return Next index in the circular buffer.
     */
    static CASTLE_CONSTEXPR size_type advance(size_type index) CASTLE_NOEXCEPT
    {
        return (index + 1U == N) ? 0U : (index + 1U);
    }

    /**
     * @brief Retreats the given index by one position in the circular buffer.
     * @param index Current index in the circular buffer.
     * @return Previous index in the circular buffer.
     */
    static CASTLE_CONSTEXPR size_type retreat(size_type index) CASTLE_NOEXCEPT
    {
        return (index == 0U) ? (N - 1U) : (index - 1U);
    }

    container::array<value_type, N> data_{};
    size_type head_ = 0U;
    size_type size_ = 0U;
};

}
}

#endif
