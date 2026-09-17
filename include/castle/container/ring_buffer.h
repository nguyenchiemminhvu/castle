#ifndef CASTLE_CONTAINER_RING_BUFFER_H
#define CASTLE_CONTAINER_RING_BUFFER_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/utility/move.h"
#include "castle/container/array.h"

namespace castle
{
namespace container
{

// Fixed-capacity FIFO circular buffer. Storage is owned by the object and
// overflow behaviour is explicit through push() and force_push().
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

    ring_buffer() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    ring_buffer(CASTLE_CONST ring_buffer&)            CASTLE_DELETE;
    ring_buffer& operator=(CASTLE_CONST ring_buffer&) CASTLE_DELETE;
    ring_buffer(ring_buffer&&)                        CASTLE_DELETE;
    ring_buffer& operator=(ring_buffer&&)             CASTLE_DELETE;

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }

    void clear() CASTLE_NOEXCEPT
    {
        head_ = 0U;
        tail_ = 0U;
        size_ = 0U;
    }

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

    // Appends value. When full, the oldest item is discarded.
    // Returns true when an item was evicted.
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

    bool peek(size_type index, value_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return false;
        }
        out = data_[wrap(head_ + index)];
        return true;
    }

    reference front() CASTLE_NOEXCEPT { return data_[head_]; }
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return data_[head_]; }
    reference back() CASTLE_NOEXCEPT { return data_[wrap(tail_ + N - 1U)]; }
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return data_[wrap(tail_ + N - 1U)]; }

    size_type push_bulk(CASTLE_CONST value_type* src, size_type max) CASTLE_NOEXCEPT
    {
        if (src == nullptr)
        {
            return 0U;
        }
        size_type written = 0U;
        while ((written < max) && !full())
        {
            data_[tail_] = src[written];
            tail_ = advance(tail_);
            ++size_;
            ++written;
        }
        return written;
    }

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
    static CASTLE_CONSTEXPR size_type wrap(size_type index) CASTLE_NOEXCEPT
    {
        return meta::is_power_of_two<N>::value
                   ? (index & (N - 1U))
                   : (index % N);
    }

    static CASTLE_CONSTEXPR size_type advance(size_type index) CASTLE_NOEXCEPT
    {
        return wrap(index + 1U);
    }

    container::array<value_type, N> data_{};
    size_type head_ = 0U;
    size_type tail_ = 0U;
    size_type size_ = 0U;
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_RING_BUFFER_H
