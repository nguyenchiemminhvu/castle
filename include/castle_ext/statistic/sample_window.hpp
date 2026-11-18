// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sample_window.hpp
 * @brief Fixed-capacity, drop-oldest window of the most recent samples.
 *
 * `sample_window<T, Capacity>` is the streaming storage shared by the sliding
 * statistics in `castle::statistic`. It keeps the newest `Capacity` samples in
 * inline storage (a `castle::dsa::ring_buffer`), evicts the oldest sample when
 * full, and reports the evicted value so incremental estimators can retract
 * it. Samples are addressed oldest-to-newest, and `latest(count)` returns a
 * lightweight view over the newest samples for sub-window statistics.
 *
 * @code
 * #include "castle_ext/statistic/sample_window.hpp"
 *
 * castle::statistic::sample_window<float, 4U> window;
 * window.push(1.0f);
 * float evicted = 0.0f;
 * if (window.push(2.0f, evicted))
 * {
 *     // `evicted` holds the sample that left the window.
 * }
 * for (const float sample : window.latest(2U))
 * {
 *     (void)sample;
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_SAMPLE_WINDOW_HPP
#define CASTLE_EXT_STATISTIC_SAMPLE_WINDOW_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/ring_buffer.hpp"
#include "castle/iterator/tags.hpp"

namespace castle
{
namespace statistic
{

/**
 * @brief Drop-oldest window over the newest `Capacity` samples.
 * @tparam T Sample type. Must be trivially copyable, trivially destructible, and default constructible.
 * @tparam Capacity Maximum number of retained samples. Must be greater than zero.
 * @note Logical index 0 is the oldest retained sample and `size() - 1` the newest.
 * @note The window is non-copyable and non-movable, like the underlying `ring_buffer`.
 * @warning Not thread-safe; protect shared instances externally.
 */
template <typename T, size_type Capacity>
class sample_window
{
    static_assert(Capacity > 0U, "castle::statistic::sample_window: Capacity must be greater than zero");

public:
    using value_type      = T;
    using size_type       = castle::size_type;
    using difference_type = castle::difference_type;

    class view;

    /**
     * @brief Read-only forward iterator over samples in oldest-to-newest order.
     * @note Dereferencing yields a copy of the sample (the ring has no contiguous storage).
     * @note Iterators are invalidated by `push()` and `clear()`.
     */
    class const_iterator
    {
    public:
        using iterator_category = castle::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = castle::difference_type;
        using pointer           = CASTLE_CONST T*;
        using reference         = T;

        /** @brief Constructs a singular iterator that must not be dereferenced. */
        const_iterator() CASTLE_NOEXCEPT
            : owner_(nullptr), index_(0U)
        {
        }

        /** @brief Returns a copy of the referenced sample. */
        reference operator*() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return (*owner_)[index_];
        }

        /** @brief Advances to the next newer sample. */
        const_iterator& operator++() CASTLE_NOEXCEPT
        {
            ++index_;
            return *this;
        }

        /** @brief Advances to the next newer sample and returns the previous position. */
        const_iterator operator++(int) CASTLE_NOEXCEPT
        {
            CASTLE_CONST const_iterator previous = *this;
            ++index_;
            return previous;
        }

        /** @brief Compares logical positions; iterators must refer to the same window. */
        friend bool operator==(CASTLE_CONST const_iterator& lhs, CASTLE_CONST const_iterator& rhs) CASTLE_NOEXCEPT
        {
            return lhs.index_ == rhs.index_;
        }

        /** @brief Negation of `operator==`. */
        friend bool operator!=(CASTLE_CONST const_iterator& lhs, CASTLE_CONST const_iterator& rhs) CASTLE_NOEXCEPT
        {
            return lhs.index_ != rhs.index_;
        }

    private:
        friend class sample_window;

        const_iterator(CASTLE_CONST sample_window* owner, size_type index) CASTLE_NOEXCEPT
            : owner_(owner), index_(index)
        {
        }

        CASTLE_CONST sample_window* owner_;
        size_type index_;
    };

    /**
     * @brief Non-owning range over a contiguous run of logical positions.
     * @note Usable in range-based `for` loops and with iterator-pair algorithms.
     * @warning Invalidated by `push()` and `clear()` on the owning window.
     */
    class view
    {
    public:
        /** @brief Iterator to the first (oldest) sample of the view. */
        const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return first_; }
        /** @brief Iterator one past the newest sample of the view. */
        const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return last_; }
        /** @brief Number of samples in the view. */
        size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
        /** @brief Reports whether the view contains no samples. */
        bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    private:
        friend class sample_window;

        view(const_iterator first, const_iterator last, size_type size) CASTLE_NOEXCEPT
            : first_(first), last_(last), size_(size)
        {
        }

        const_iterator first_;
        const_iterator last_;
        size_type size_;
    };

    /** @brief Constructs an empty window. Complexity: O(1). */
    sample_window() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /** @brief Returns the compile-time capacity. */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return Capacity; }

    /** @brief Returns the number of retained samples. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return buffer_.size(); }

    /** @brief Reports whether no sample is retained. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return buffer_.empty(); }

    /** @brief Reports whether the next `push()` evicts the oldest sample. */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return buffer_.full(); }

    /** @brief Returns `capacity() - size()`. */
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return buffer_.available(); }

    /**
     * @brief Appends a sample, evicting the oldest one when the window is full.
     * @param value Sample to append as the newest element.
     * @return `true` when a sample was evicted, otherwise `false`.
     * @note Complexity: O(1) time and stack.
     */
    bool push(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        return buffer_.force_push(value);
    }

    /**
     * @brief Appends a sample and reports the evicted sample, if any.
     * @param value Sample to append as the newest element.
     * @param evicted Receives the evicted oldest sample; unchanged when nothing is evicted.
     * @return `true` when a sample was evicted, otherwise `false`.
     * @note Complexity: O(1) time and stack.
     */
    bool push(CASTLE_CONST T& value, T& evicted) CASTLE_NOEXCEPT
    {
        CASTLE_CONST bool evicting = buffer_.full();
        if (evicting)
        {
            evicted = buffer_.front();
        }
        (void)buffer_.force_push(value);
        return evicting;
    }

    /** @brief Removes every sample. Complexity: O(1). */
    void clear() CASTLE_NOEXCEPT
    {
        buffer_.clear();
    }

    /**
     * @brief Returns a copy of the sample at a logical position.
     * @param index Oldest-to-newest position; must be less than `size()`.
     * @return The sample, or a value-initialized `T` when `index` is out of range.
     * @note Complexity: O(1).
     */
    T operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        T value{};
        (void)buffer_.peek(index, value);
        return value;
    }

    /**
     * @brief Copies the sample at a logical position with bounds checking.
     * @param index Oldest-to-newest position.
     * @param out Receives the sample on success; unchanged on failure.
     * @return `status::ok` or `status::out_of_range`.
     * @note Complexity: O(1).
     */
    castle::status at(size_type index, T& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return buffer_.peek(index, out) ? castle::status::ok : castle::status::out_of_range;
    }

    /**
     * @brief Copies the oldest retained sample.
     * @param out Receives the sample on success; unchanged on failure.
     * @return `status::ok` or `status::empty`.
     * @note Complexity: O(1).
     */
    castle::status oldest(T& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (buffer_.empty())
        {
            return castle::status::empty;
        }
        out = buffer_.front();
        return castle::status::ok;
    }

    /**
     * @brief Copies the newest retained sample.
     * @param out Receives the sample on success; unchanged on failure.
     * @return `status::ok` or `status::empty`.
     * @note Complexity: O(1).
     */
    castle::status newest(T& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (buffer_.empty())
        {
            return castle::status::empty;
        }
        out = buffer_.back();
        return castle::status::ok;
    }

    /** @brief Iterator to the oldest sample. */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(this, 0U); }

    /** @brief Iterator one past the newest sample. */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(this, size()); }

    /**
     * @brief Returns a view over the newest `count` samples (oldest-to-newest order).
     * @param count Requested sample count; clamped to `size()`.
     * @return View over `min(count, size())` samples.
     * @note Complexity: O(1).
     */
    view latest(size_type count) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type retained = size();
        CASTLE_CONST size_type taken = (count < retained) ? count : retained;
        return view(const_iterator(this, retained - taken), end(), taken);
    }

    /**
     * @brief Copies samples in oldest-to-newest order into caller-owned storage.
     * @param destination Destination array; may be `nullptr` only to copy nothing.
     * @param destination_capacity Number of writable elements at `destination`.
     * @return Number of samples copied: `min(size(), destination_capacity)`, or 0 for `nullptr`.
     * @note Complexity: O(size()).
     */
    size_type copy_to(T* destination, size_type destination_capacity) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (destination == nullptr)
        {
            return 0U;
        }
        CASTLE_CONST size_type retained = size();
        CASTLE_CONST size_type count = (retained < destination_capacity) ? retained : destination_capacity;
        for (size_type i = 0U; i < count; ++i)
        {
            (void)buffer_.peek(i, destination[i]);
        }
        return count;
    }

private:
    castle::dsa::ring_buffer<T, Capacity> buffer_{};
};

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_SAMPLE_WINDOW_HPP
