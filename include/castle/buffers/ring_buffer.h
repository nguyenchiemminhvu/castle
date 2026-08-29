#pragma once

#include "castle/bit/bit_math.h"
#include "castle/iterators/iterator.h"

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

using namespace castle::bit;

// ─────────────────────────────────────────────────────────────────────────────
// ring_buffer<T, N> — stack-allocated, bounded, FIFO circular buffer.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * A fixed-capacity ring (circular) buffer intended for safety-critical
 * embedded targets with tightly constrained resources.
 *
 *   • Capacity N is a compile-time constant. All storage lives inside the
 *     object; there is NO heap allocation, NO exceptions thrown, and NO RTTI
 *     usage.  Element type T must be trivially copyable so that no user
 *     defined constructor / destructor can throw or allocate.
 *   • FIFO semantics: push() appends to the tail, pop() removes from the head.
 *   • Two overflow policies are provided explicitly (MISRA-friendly — no
 *     hidden behaviour):
 *         - push(v)        : returns false when full, buffer untouched.
 *         - force_push(v)  : always succeeds; on overflow the oldest element
 *                            is silently dropped (drop-oldest / overwrite).
 *   • All observers and mutators are noexcept. No exceptions are ever
 *     thrown; error signalling uses return values / booleans.
 *   • NOT thread-safe on its own. For SPSC use, wrap accesses with the
 *     appropriate memory barriers at the call site, or serialise with a
 *     mutex in MPMC scenarios.
 */

namespace castle
{
namespace buffers
{

template <typename T, std::size_t N>
class ring_buffer
{
    static_assert(N > 0, "ring_buffer capacity must be non-zero");
    static_assert(std::is_trivially_copyable<T>::value,
                  "ring_buffer<T,N> requires T to be trivially copyable "
                  "so that no allocation / exception can occur on copy");
    static_assert(std::is_trivially_destructible<T>::value,
                  "ring_buffer<T,N> requires T to be trivially destructible");

public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;

    using self_type = ring_buffer<T, N>;

    // The iterator walks the logical FIFO order (oldest -> newest), never the
    // physical array layout. Random-access because iterator_at() is O(1).
    using iterator =
        castle::iterators::basic_iterator<
            self_type,
            castle::iterators::iterator_at_traversal,
            castle::iterators::random_access_iterator_tag>;
    using const_iterator =
        castle::iterators::basic_iterator<
            const self_type,
            castle::iterators::iterator_at_traversal,
            castle::iterators::random_access_iterator_tag>;
    using reverse_iterator       = castle::iterators::reverse_iterator<iterator>;
    using const_reverse_iterator = castle::iterators::reverse_iterator<const_iterator>;

    ring_buffer() noexcept = default;

    // Non-copyable / non-movable: a ring buffer usually represents a shared
    // stream endpoint. Enforce explicit intent at the call site.
    ring_buffer(const ring_buffer&)            = delete;
    ring_buffer& operator=(const ring_buffer&) = delete;
    ring_buffer(ring_buffer&&)                 = delete;
    ring_buffer& operator=(ring_buffer&&)      = delete;

    // ── capacity / state ────────────────────────────────────────────────

    static constexpr size_type capacity() noexcept { return N; }

    size_type size()      const noexcept { return size_; }
    bool      empty()     const noexcept { return size_ == 0; }
    bool      full()      const noexcept { return size_ == N; }
    size_type available() const noexcept { return N - size_; }

    void clear() noexcept
    {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

    // ── producer side ───────────────────────────────────────────────────

    /**
     * Append @p v to the tail.
     * @return true on success, false when the buffer is already full
     *         (in which case the buffer is left untouched).
     */
    bool push(const value_type& v) noexcept
    {
        if (size_ == N)
        {
            return false;
        }

        data_[tail_] = v;
        tail_        = advance(tail_);
        ++size_;
        return true;
    }

    bool push(value_type&& v) noexcept
    {
        if (size_ == N)
        {
            return false;
        }

        data_[tail_] = std::move(v);
        tail_        = advance(tail_);
        ++size_;
        return true;
    }

    /**
     * Always succeeds. If the buffer is full, the oldest element is dropped
     * (overwrite policy). Returns true when an element had to be evicted so
     * the caller can account for data loss.
     */
    bool force_push(const value_type& v) noexcept
    {
        const bool evicted = (size_ == N);
        if (evicted)
        {
            head_ = advance(head_);
            --size_;
        }

        data_[tail_] = v;
        tail_        = advance(tail_);
        ++size_;
        return evicted;
    }

    /**
     * Always succeeds. If the buffer is full, the oldest element is dropped
     * (overwrite policy). Returns true when an element had to be evicted so
     * the caller can account for data loss.
     */
    bool force_push(value_type&& v) noexcept
    {
        const bool evicted = (size_ == N);
        if (evicted)
        {
            head_ = advance(head_);
            --size_;
        }

        data_[tail_] = std::move(v);
        tail_        = advance(tail_);
        ++size_;
        return evicted;
    }

    // ── consumer side ───────────────────────────────────────────────────

    /**
     * Remove the head element and copy it into @p out.
     * @return true on success, false when the buffer is empty
     *         (@p out is left untouched in that case).
     */
    bool pop(value_type& out) noexcept
    {
        if (size_ == 0)
        {
            return false;
        }

        out   = data_[head_];
        head_ = advance(head_);
        --size_;
        return true;
    }

    /**
     * Drop the head element without returning it.
     * @return true on success, false when empty.
     */
    bool pop() noexcept
    {
        if (size_ == 0)
        {
            return false;
        }

        head_ = advance(head_);
        --size_;
        return true;
    }

    // ── inspection (UB-free, bounded) ───────────────────────────────────

    /**
     * Peek at position @p index from the head (0 == oldest, size()-1 == newest).
     * @return true on success, false when @p index is out of range.
     */
    bool peek(size_type index, value_type& out) const noexcept
    {
        if (index >= size_)
        {
            return false;
        }

        out = data_[wrap(head_ + index)];
        return true;
    }

    /**
     * Access the oldest element. Precondition: !empty().
     * Calling on an empty buffer is a programming error (undefined behaviour).
     */
    reference       front()       noexcept { return data_[head_]; }
    const_reference front() const noexcept { return data_[head_]; }

    /**
     * Access the newest element. Precondition: !empty().
     */
    reference       back()       noexcept { return data_[wrap(tail_ + N - 1U)]; }
    const_reference back() const noexcept { return data_[wrap(tail_ + N - 1U)]; }

    // ── bulk operations ─────────────────────────────────────────────────

    /**
     * Copy up to @p max elements from a contiguous source into the buffer
     * using the strict push() policy (no overwrite).
     * @return number of elements actually written.
     */
    size_type push_bulk(const value_type* src, size_type max) noexcept
    {
        if (src == nullptr)
        {
            return 0;
        }

        size_type written = 0;
        while (written < max && size_ < N)
        {
            data_[tail_] = src[written];
            tail_        = advance(tail_);
            ++size_;
            ++written;
        }
        return written;
    }

    /**
     * Drain up to @p max elements from the head into @p dst.
     * @return number of elements actually copied out.
     */
    size_type pop_bulk(value_type* dst, size_type max) noexcept
    {
        if (dst == nullptr)
        {
            return 0;
        }

        size_type read = 0;
        while (read < max && size_ > 0)
        {
            dst[read] = data_[head_];
            head_     = advance(head_);
            --size_;
            ++read;
        }
        return read;
    }

    // ── iteration (logical FIFO order) ────────────────────────────────
    //
    // begin() -> oldest element, end() -> one-past newest. Any mutating
    // operation invalidates existing iterators.

    iterator       begin()       noexcept { return iterator(this, 0); }
    iterator       end()         noexcept { return iterator(this, static_cast<difference_type>(size_)); }
    const_iterator begin() const noexcept { return const_iterator(this, 0); }
    const_iterator end()   const noexcept { return const_iterator(this, static_cast<difference_type>(size_)); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end(); }

    reverse_iterator       rbegin()       noexcept { return reverse_iterator(end()); }
    reverse_iterator       rend()         noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend()   const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend()   const noexcept { return const_reverse_iterator(cbegin()); }

    // Maps a logical index (0 == oldest) to its element. Used by the iterator
    // traversal policy. Precondition: index < size(); no bounds check so that
    // dereferencing a valid iterator stays O(1) and branch-free.
    reference       iterator_at(size_type index)       noexcept { return data_[wrap(head_ + index)]; }
    const_reference iterator_at(size_type index) const noexcept { return data_[wrap(head_ + index)]; }

private:
    static constexpr size_type wrap(size_type i) noexcept
    {
        // Compile-time picks a cheap mask when N is a power of two; otherwise
        // the compiler emits a single modulo. Both are branch-free.
        return is_power_of_two_v<N> ? (i & (N - 1U)) : (i % N);
    }

    static constexpr size_type advance(size_type i) noexcept
    {
        return wrap(i + 1U);
    }

    std::array<value_type, N> data_{};
    size_type                 head_ = 0;   // index of the oldest element
    size_type                 tail_ = 0;   // index where the next element will be written
    size_type                 size_ = 0;   // number of stored elements (0 .. N)
};

} // namespace buffers
} // namespace castle

