// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ring_buffer.hpp
 * @brief Fixed-capacity FIFO buffer with circular indexing.
 *
 * Use `ring_buffer<T, N>` when items must be queued in first-in/first-out
 * order with deterministic memory usage. Storage is embedded, capacity is
 * fixed by `N`, and all single-element operations are O(1). `push()` reports
 * a full buffer without overwriting, while `force_push()` explicitly evicts the
 * oldest element when necessary.
 *
 * @code
 * castle::container::ring_buffer<int, 4U> fifo;
 * fifo.push(1);
 * fifo.force_push(2);
 * int value = fifo.front();
 * @endcode
 */
#ifndef CASTLE_CONTAINER_RING_BUFFER_HPP
#define CASTLE_CONTAINER_RING_BUFFER_HPP

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
 * @brief Fixed-capacity FIFO buffer with inline storage.
 * @tparam T Element type. Must be trivially copyable and trivially destructible.
 * @tparam N Queue capacity. Must be greater than `0`.
 * @note `push()`, `force_push()`, `pop()`, `peek()`, `front()`, and `back()`
 * are O(1). Bulk operations are O(k) where `k` is the number of transferred elements.
 * @note The container has no iterators. References returned by `front()` and
 * `back()` remain valid only until the next mutating operation that removes or overwrites them.
 * @warning `front()` and `back()` require the buffer to be non-empty.
 */
template <typename T, size_type N>
class ring_buffer
{
    static_assert(N > 0U, "ring_buffer capacity must be non-zero");
    static_assert(meta::is_trivially_copyable<T>::value,
                  "ring_buffer<T,N> requires T to be trivially copyable");
    static_assert(meta::is_trivially_destructible<T>::value,
                  "ring_buffer<T,N> requires T to be trivially destructible");

public:
    using value_type      = T;
    using size_type       = castle::size_type;
    using difference_type = castle::difference_type;
    using reference       = T&;
    using const_reference = CASTLE_CONST T&;

    /**
     * @brief Constructs an empty ring buffer.
     * @note Complexity: O(1).
     */
    ring_buffer() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Constructs a ring buffer by pushing initializer-list elements in order.
     * @param list Values appended from first to last.
     * @note Complexity: O(list.size()).
     * @warning `list.size()` must not exceed `N`; the constructor reports a
     * violated precondition with `CASTLE_ASSERT`.
     */
    ring_buffer(initializer_list<T> list) CASTLE_NOEXCEPT
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
     * @param other Source buffer.
     * @warning Ring buffers are non-copyable.
     */
    ring_buffer(CASTLE_CONST ring_buffer&)            CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @param other Source buffer.
     * @return This declaration is deleted.
     * @warning Ring buffers are non-copyable.
     */
    ring_buffer& operator=(CASTLE_CONST ring_buffer&) CASTLE_DELETE;

    /**
     * @brief Move construction is disabled.
     * @param other Source buffer.
     * @warning Ring buffers are non-movable.
     */
    ring_buffer(ring_buffer&&)                        CASTLE_DELETE;

    /**
     * @brief Move assignment is disabled.
     * @param other Source buffer.
     * @return This declaration is deleted.
     * @warning Ring buffers are non-movable.
     */
    ring_buffer& operator=(ring_buffer&&)             CASTLE_DELETE;

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
     * @brief Reports whether the buffer is empty.
     * @return `true` when `size() == 0`.
     * @note Complexity: O(1).
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Reports whether the buffer is full.
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
     * @warning References to stored elements become invalid.
     */
    void clear() CASTLE_NOEXCEPT
    {
        head_ = 0U;
        tail_ = 0U;
        size_ = 0U;
    }

    /**
     * @brief Appends a copied value when capacity is available.
     * @param value Value to append at the back of the queue.
     * @return `true` on success or `false` when the buffer is full.
     * @note Complexity: O(1).
     */
    bool push(CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return false;
        }
        data_[tail_] = value;
        tail_ = advance(tail_);
        ++size_;
        return true;
    }

    /**
     * @brief Appends a moved value when capacity is available.
     * @param value Value to move into the back of the queue.
     * @return `true` on success or `false` when the buffer is full.
     * @note Complexity: O(1).
     */
    bool push(value_type&& value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return false;
        }
        data_[tail_] = castle::move(value);
        tail_ = advance(tail_);
        ++size_;
        return true;
    }

    /**
     * @brief Appends a copied value, evicting the oldest element when full.
     * @param value Value to append at the back of the queue.
     * @return `true` when an existing element was evicted, otherwise `false`.
     * @note Complexity: O(1).
     */
    bool force_push(CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST bool evicted = full();
        if (evicted)
        {
            head_ = advance(head_);
            --size_;
        }
        data_[tail_] = value;
        tail_ = advance(tail_);
        ++size_;
        return evicted;
    }

    /**
     * @brief Appends a moved value, evicting the oldest element when full.
     * @param value Value to move into the back of the queue.
     * @return `true` when an existing element was evicted, otherwise `false`.
     * @note Complexity: O(1).
     */
    bool force_push(value_type&& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST bool evicted = full();
        if (evicted)
        {
            head_ = advance(head_);
            --size_;
        }
        data_[tail_] = castle::move(value);
        tail_ = advance(tail_);
        ++size_;
        return evicted;
    }

    /**
     * @brief Removes the front element and copies it to `out`.
     * @param out Destination for the removed value.
     * @return `true` on success or `false` when the buffer is empty.
     * @note Complexity: O(1).
     */
    bool pop(value_type& out) CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return false;
        }
        out = data_[head_];
        head_ = advance(head_);
        --size_;
        return true;
    }

    /**
     * @brief Removes the front element and discards it.
     * @return `true` on success or `false` when the buffer is empty.
     * @note Complexity: O(1).
     */
    bool pop() CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return false;
        }
        head_ = advance(head_);
        --size_;
        return true;
    }

    /**
     * @brief Copies an element at logical position `index` without removing it.
     * @param index Zero-based index from the front of the queue.
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
        out = data_[wrap(head_ + index)];
        return true;
    }

    /**
     * @brief Returns the oldest stored element.
     * @return Mutable reference to the front element.
     * @note Complexity: O(1).
     * @warning The buffer must not be empty.
     */
    reference front() CASTLE_NOEXCEPT { return data_[head_]; }

    /**
     * @brief Returns the oldest stored element.
     * @return Const reference to the front element.
     * @note Complexity: O(1).
     * @warning The buffer must not be empty.
     */
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return data_[head_]; }

    /**
     * @brief Returns the newest stored element.
     * @return Mutable reference to the back element.
     * @note Complexity: O(1).
     * @warning The buffer must not be empty.
     */
    reference back() CASTLE_NOEXCEPT { return data_[retreat(tail_)]; }

    /**
     * @brief Returns the newest stored element.
     * @return Const reference to the back element.
     * @note Complexity: O(1).
     * @warning The buffer must not be empty.
     */
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return data_[retreat(tail_)]; }

    /**
     * @brief Appends up to `max` values from `src`.
     * @param src Source array of values to append.
     * @param max Maximum number of values to append.
     * @return Number of values actually appended.
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
            data_[tail_] = src[written];
            tail_ = advance(tail_);
            ++size_;
            ++written;
        }
        return written;
    }

    /**
     * @brief Removes up to `max` values from the front into `dst`.
     * @param dst Destination array for removed values.
     * @param max Maximum number of values to remove.
     * @return Number of values actually removed.
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
            dst[read] = data_[head_];
            head_ = advance(head_);
            --size_;
            ++read;
        }
        return read;
    }

private:
    /**
     * @brief Wraps the given index according to the buffer size.
     * @param index Index to be wrapped.
     * @return Wrapped index within the buffer size.
     */
    static CASTLE_CONSTEXPR size_type wrap(size_type index) CASTLE_NOEXCEPT
    {
        return meta::is_power_of_two<N>::value
                   ? (index & (N - 1U))
                   : (index % N);
    }

    /**
     * @brief Advances the given index by one position, wrapping around if necessary.
     * @param index Index to be advanced.
     * @return Advanced index within the buffer size.
     */
    static CASTLE_CONSTEXPR size_type advance(size_type index) CASTLE_NOEXCEPT
    {
        return (index + 1U == N) ? 0U : (index + 1U);
    }

    /**
     * @brief Retreats the given index by one position, wrapping around if necessary.
     * @param index Index to be retreated.
     * @return Retreated index within the buffer size.
     */
    static CASTLE_CONSTEXPR size_type retreat(size_type index) CASTLE_NOEXCEPT
    {
        return (index == 0U) ? (N - 1U) : (index - 1U);
    }

    container::array<value_type, N> data_{};
    size_type head_ = 0U;
    size_type tail_ = 0U;
    size_type size_ = 0U;
};

}
}

#endif
